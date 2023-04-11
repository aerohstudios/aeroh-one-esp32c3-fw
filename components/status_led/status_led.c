#include "led_strip.h"
#include "logging.h"
#include "errors.h"
#include "state_machine.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BLINK_GPIO  6
#define BLINK_RMT_CHANNEL 0

#define MAX_LED_BRIGHTNESS 31

static led_strip_t *pStrip_a;
static int32_t current_state = MACHINE_STATE_EMPTY;

struct ColorIntensity {
    int red;
    int green;
    int blue;
} color_intensity;

static void set_color_intensity(int r, int g, int b) {
    color_intensity.red = r;
    color_intensity.green = g;
    color_intensity.blue = b;
}

static void initialize(void) {
    LOGI("Configuring Addressable LED");
    set_color_intensity(0, 0, 0);
    pStrip_a = led_strip_init(BLINK_RMT_CHANNEL, BLINK_GPIO, 1);
    pStrip_a->clear(pStrip_a, 50);
}

void set_pixel(int brightness) {
    pStrip_a->set_pixel(
        pStrip_a,
        0,
        color_intensity.red * brightness,
        color_intensity.green * brightness,
        color_intensity.blue * brightness
    );
    pStrip_a->refresh(pStrip_a, 100);
}

bool machine_state_changed(int msToWait) {
    LOGI("Checking if Machine State Changed!");

    vTaskDelay(pdMS_TO_TICKS(msToWait));
    if (current_state != get_current_state_from_ram()) {
        LOGI("Machine State Changed!");
        return true;
    } else {
        LOGI("Machine State Didn't Changed!");
        return false;
    }
}

void strobe(void) {
    while (1) {
        for (int i = 1; i <= MAX_LED_BRIGHTNESS; i=i+2) {
            set_pixel(i);
            vTaskDelay(pdMS_TO_TICKS(30));
        }

        for (int i = MAX_LED_BRIGHTNESS; i >= 0; i=i-2) {
            set_pixel(i);
            vTaskDelay(pdMS_TO_TICKS(30));
        }

        // break when status change
        if (machine_state_changed(1)) {
            break;
        }
    }
}

void blink(int on_ms, int off_ms) {
    while (1) {
        set_pixel(MAX_LED_BRIGHTNESS);
        vTaskDelay(pdMS_TO_TICKS(on_ms));
        set_pixel(0);
        vTaskDelay(pdMS_TO_TICKS(off_ms));

        // break when status change
        if (machine_state_changed(1)) {
            break;
        }
    }
}

void slow_blink(void) {
    blink(150, 850);
}

void rapid_blink(void) {
    blink(150, 150);
}

void solid(void) {
    set_pixel(MAX_LED_BRIGHTNESS);
    vTaskDelay(pdMS_TO_TICKS(100));
    // Block till status change or timeout
    if (machine_state_changed(1000)) {
        return;
    }
}

void clear(void) {
    pStrip_a->clear(pStrip_a, 50);

    // Block till status change or timeout
    if (machine_state_changed(1000)) {
        return;
    }
}

void display_startup_error(void) {
    LOGI("Status LED: Showing Startup Error");
    initialize();
    while (1) {
        set_color_intensity(1, 0, 0);
        solid();
    }
}

void vStatusLEDTask(void *pvParameters) {
    initialize();

    while(1) {
        LOGI("STATUS LED State Update: %d", get_current_state_from_ram());

        current_state = get_current_state_from_ram();

        switch(current_state) {
            case MACHINE_STATE_EMPTY:
            case MACHINE_STATE_NEW:
                set_color_intensity(0, 0, 1);
                strobe();
                break;

            case MACHINE_STATE_PROVISIONING_BT_CONNECTED:
                set_color_intensity(0, 0, 1);
                solid();
                break;
            case MACHINE_STATE_PROVISIONING_BT_TRANSFER:
                set_color_intensity(0, 0, 1);
                rapid_blink();
                break;
            case MACHINE_STATE_PROVISIONING_WIFI_CONNECTING:
            case MACHINE_STATE_STARTUP_WIFI_CONNECTING:
                set_color_intensity(1, 1, 1);
                strobe();
                break;

            case MACHINE_STATE_PROVISIONING_WIFI_CONNECTED:
            case MACHINE_STATE_STARTUP_WIFI_CONNECTED:
                set_color_intensity(1, 1, 1);
                solid();
                break;
            case MACHINE_STATE_PROVISIONING_WIFI_CONNECTED_BT_TRANSFER:
                set_color_intensity(0, 0, 1);
                rapid_blink();
                break;

            case MACHINE_STATE_PROVISIONING_MQTT_CONNECTING:
            case MACHINE_STATE_STARTUP_MQTT_CONNECTING:
                set_color_intensity(1, 1, 1);
                rapid_blink();
                break;
            case MACHINE_STATE_PROVISIONING_CONNECTED:
            case MACHINE_STATE_STARTUP_MQTT_CONNECTED:
                set_color_intensity(0, 1, 0);
                solid();
                break;

            case MACHINE_STATE_TRAINING_READY:
                set_color_intensity(1, 0, 1);
                strobe();
                break;
            case MACHINE_STATE_TRAINING_DOWNLOADING:
                set_color_intensity(1, 0, 1);
                rapid_blink();
                break;
            case MACHINE_STATE_TRAINING_COMPLETE:
                set_color_intensity(0, 1, 0);
                solid();
                break;
            case MACHINE_STATE_TRAINING_RECORD_READY:
                set_color_intensity(1, 0, 1);
                solid();
                break;
            case MACHINE_STATE_TRAINING_RECORDING:
                set_color_intensity(1, 1, 0);
                solid();
                break;
            case MACHINE_STATE_TRAINING_RECORDED:
                set_color_intensity(1, 0, 1);
                rapid_blink();
                break;

            case MACHINE_STATE_ACTIVE_DUTY_CONNECTED:
                set_color_intensity(0, 0, 0);
                clear();
                break;
            case MACHINE_STATE_ACTIVE_DUTY_RUNNING_CMD:
                set_color_intensity(1, 0, 0);
                solid();
                break;

            case MACHINE_STATE_RESET_BUTTON_HOLD:
                set_color_intensity(0, 1, 1);
                slow_blink();
                break;
            case MACHINE_STATE_RESET_CLEAR_WIFI:
                set_color_intensity(0, 1, 1);
                rapid_blink();
                break;
            case MACHINE_STATE_RESET_CLEAR_WIFI_MQTT:
                set_color_intensity(0, 1, 1);
                rapid_blink();
                break;
            case MACHINE_STATE_RESET_CLEAR_WIFI_MQTT_TRAINING:
                set_color_intensity(0, 1, 1);
                rapid_blink();
                break;

            default:
                LOGE("Status LED failed to react to current state.");
        }
    }

    vTaskDelete( NULL );
}
