/*
 * ir_repeater.c
 *
 *  Created on: Aug 17, 2022
 *      Author: shiv
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

static gpio_num_t intr_gpio_output_pin;

static void IRAM_ATTR gpio_isr_handler(void *gpio_input_pin) {
    gpio_set_level(intr_gpio_output_pin, 1^gpio_get_level((gpio_num_t) gpio_input_pin));
}

void initialize_repeater(gpio_num_t gpio_input_pin, gpio_num_t gpio_output_pin) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_direction(gpio_output_pin, GPIO_MODE_OUTPUT));
    intr_gpio_output_pin = gpio_output_pin;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_direction(gpio_input_pin, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_intr_type(gpio_input_pin, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_isr_handler_add(gpio_input_pin, gpio_isr_handler, (void*)gpio_input_pin));
}

void clear_repeater(gpio_num_t gpio_input_pin, gpio_num_t gpio_output_pin) {
	ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_isr_handler_remove(gpio_input_pin));
	gpio_uninstall_isr_service();
	ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_intr_type(gpio_input_pin, GPIO_INTR_DISABLE));
}
