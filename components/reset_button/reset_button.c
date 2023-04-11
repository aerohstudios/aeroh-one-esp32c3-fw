#include "logging.h"
#include "errors.h"
#include "state_machine.h"
#include "storage.h"
#include "status_led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_wifi.h"

#define GPIO_BUTTON_INPUT_PIN 5

#define PRESS_DURATION_THRESHOLD_MS (15*1000)
#define SLEEP_DURATION_MS 100

static int button_pressed = 0;

static void IRAM_ATTR reset_button_interrupt_handler(void *args)
{
    int pinNumber = (int) args;

    if (gpio_get_level(pinNumber) == 1) {
        button_pressed = 1;
    } else {
        button_pressed = 0;
    }
}

void system_shutdown_handler() {
	tearDownStatusLEDs();
}

void vResetButtonTask(void *pvParameters) {
	LOGI("Setting up reset button handlers!");
	gpio_pad_select_gpio(GPIO_BUTTON_INPUT_PIN);
	gpio_set_direction(GPIO_BUTTON_INPUT_PIN, GPIO_MODE_INPUT);
	gpio_pulldown_en(GPIO_BUTTON_INPUT_PIN);
	gpio_pullup_dis(GPIO_BUTTON_INPUT_PIN);
	gpio_set_intr_type(GPIO_BUTTON_INPUT_PIN, GPIO_INTR_ANYEDGE);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(GPIO_BUTTON_INPUT_PIN, reset_button_interrupt_handler, (void*) GPIO_BUTTON_INPUT_PIN);

	// set reset handler before resetting the device
	esp_register_shutdown_handler(system_shutdown_handler);

	// if the button was already pressed before ISR Picks up
	// for example on boot
	if (gpio_get_level(GPIO_BUTTON_INPUT_PIN) == 1) {
		button_pressed = 1;
	}

	int btn_pressed_ctr = 0;
	while (1) {
		if (button_pressed == 1) {
			btn_pressed_ctr += 1;
			LOGI("Reset Button held for %d ms!", btn_pressed_ctr * SLEEP_DURATION_MS);
		} else {
			btn_pressed_ctr = 0;
		}

		if (btn_pressed_ctr >= (PRESS_DURATION_THRESHOLD_MS/SLEEP_DURATION_MS)) {
			set_state_machine_state(MACHINE_STATE_RESET_CLEAR_WIFI_MQTT);
			LOGI("Resetting Device!");
			LOGI("Removing Wifi Credentials...");
			esp_wifi_restore();
			LOGI("Done!!! Removing MQTT Credentials...");
			storage_reset_mqtt_credentials();
			LOGI("Done!!! Give LED Feedback for another 3 Seconds!");
			vTaskDelay(pdMS_TO_TICKS(3000));
			LOGI("Done!!! Resetting State Machine...");
			state_machine_reset();
			LOGI("Done!!! Restarting Device!");
			esp_restart();
		}
		vTaskDelay(pdMS_TO_TICKS(SLEEP_DURATION_MS));
	}
	vTaskDelete( NULL );
}
