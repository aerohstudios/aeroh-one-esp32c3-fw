/*
 * iris_record.c
 *
 *  Created on: Aug 16, 2022
 *      Author: shiv
 */
#include "iris_record.h"

#include "logging.h"
#include "errors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define MAX_RECORD_CAPACITY 128

#define HIGH	1
#define LOW		1

static volatile QueueHandle_t signal_pair_queue;

static volatile unsigned int duty_cycle = 0;
static volatile unsigned int frequency = 0;
static volatile unsigned long long int pwm_total_duration = 0;

static volatile bool detect_pwm_start = false;
static volatile bool detect_pwm_flipped = false;
static volatile bool detect_pwm_end = false;

static volatile unsigned long long int detect_pwm_start_time = 0;
static volatile unsigned long long int detect_pwm_flip_time = 0;

static volatile unsigned long long int current_isr_trigger_time = 0;
static volatile unsigned long long int previous_isr_trigger_time = 0;

#define UNDEFINED 2
static volatile int effective_level = UNDEFINED;
static volatile unsigned long long int effective_level_set_time = 0;

static void IRAM_ATTR record_isr_handle_func(void *gpio_input_pin) {
	current_isr_trigger_time = (unsigned long long int) esp_timer_get_time();
	int level = 1^gpio_get_level((gpio_num_t) gpio_input_pin);

	if (effective_level == UNDEFINED) {
		effective_level = level;
		effective_level_set_time = current_isr_trigger_time;
	}

	if (detect_pwm_end == false) {
		if (level == HIGH) {
			if (detect_pwm_start == false) {
				detect_pwm_start = true;
				detect_pwm_start_time = current_isr_trigger_time;
			} else if (detect_pwm_flipped == true) {
				detect_pwm_start = false;
				detect_pwm_flipped = false;
				detect_pwm_end = true;

				pwm_total_duration = current_isr_trigger_time - detect_pwm_start_time;
				duty_cycle = (int) (( (detect_pwm_flip_time - detect_pwm_start_time) / ((double) pwm_total_duration) ) * 100);
				frequency = (int) ( 1000000 / pwm_total_duration ) ;
			}
		} else {
			if (detect_pwm_start == true) {
				if (detect_pwm_flipped == false) {
					detect_pwm_flipped = true;
					detect_pwm_flip_time = current_isr_trigger_time;
				}
			}
		}
	} else {
		if ( (current_isr_trigger_time - previous_isr_trigger_time) > pwm_total_duration ) {
			signal_pair_t current_signal_pair;
			current_signal_pair.high_time = (unsigned int) previous_isr_trigger_time - effective_level_set_time;
			current_signal_pair.low_time = (unsigned int) current_isr_trigger_time - previous_isr_trigger_time;
			effective_level_set_time = current_isr_trigger_time;
			xQueueSendFromISR(signal_pair_queue, &current_signal_pair, NULL);
		}
	}
	previous_isr_trigger_time = current_isr_trigger_time;
}

void initialize_static_variables() {
	duty_cycle = 0;
	frequency = 0;
	pwm_total_duration = 0;

	detect_pwm_start = false;
	detect_pwm_flipped = false;
	detect_pwm_end = false;

	detect_pwm_start_time = 0;
	detect_pwm_flip_time = 0;

	current_isr_trigger_time = 0;
	previous_isr_trigger_time = 0;

	effective_level = UNDEFINED;
	effective_level_set_time = 0;
}

void initialize_for_record(gpio_num_t gpio_input_pin) {
	LOGI("Initialize for Recording");
	signal_pair_queue = xQueueCreate(MAX_RECORD_CAPACITY, sizeof(signal_pair_t));
	initialize_static_variables();
	ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_direction(gpio_input_pin, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_intr_type(gpio_input_pin, GPIO_INTR_ANYEDGE));
	ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
	ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_isr_handler_add(gpio_input_pin, record_isr_handle_func, (void *)gpio_input_pin));
	LOGI("Done!");
}

void cleanup_from_record(gpio_num_t gpio_input_pin) {
	LOGI("Cleanup from Recording");
	gpio_isr_handler_remove(gpio_input_pin);
	gpio_uninstall_isr_service();
	gpio_set_intr_type(gpio_input_pin, GPIO_INTR_DISABLE);
	vQueueDelete(signal_pair_queue);
	LOGI("Done!");
}

void record_for(int duration_in_ms, ir_command_t * ir_command) {
	LOGI("Allocating Memory for Recording");
	ir_command->signal_pairs = malloc(MAX_RECORD_CAPACITY * sizeof(signal_pair_t));
	int signal_pair_idx = 0;

	LOGI("Ready to record for next %d ms", duration_in_ms);
	unsigned long long int receiver_timer_start = esp_timer_get_time();
	while(1) {
		if ((esp_timer_get_time() - receiver_timer_start)  > duration_in_ms*1000) {
			if (signal_pair_idx < MAX_RECORD_CAPACITY && signal_pair_idx > 0) {
				ir_command->signal_pairs[signal_pair_idx].high_time = (unsigned int) (previous_isr_trigger_time - effective_level_set_time);
				ir_command->signal_pairs[signal_pair_idx].low_time = (unsigned int) ((unsigned long long int) esp_timer_get_time() - previous_isr_trigger_time);
				signal_pair_idx++;
			}
			LOGI("Breaking because time has expired");
			break;
		}

		signal_pair_t received_signal_pair;
		if (xQueueReceive(signal_pair_queue, &received_signal_pair, 200) == pdTRUE) {
			if (signal_pair_idx < MAX_RECORD_CAPACITY) {
				if (signal_pair_idx == 0) {
					ir_command->duty_cycle = duty_cycle;
					ir_command->frequency = frequency;
				}
				ir_command->signal_pairs[signal_pair_idx].high_time = received_signal_pair.high_time;
				ir_command->signal_pairs[signal_pair_idx].low_time = received_signal_pair.low_time;
				signal_pair_idx++;
			} else {
				LOGI("Breaking as we got enough items");
				break;
			}
		} else {
			vTaskDelay(200 / portTICK_PERIOD_MS);
		}
	}
	ir_command->length = signal_pair_idx;
	LOGI("Done recording!");
	LOGI("Got %d items with %d hz frequency and %d duty cycle", ir_command->length, ir_command->frequency, ir_command->duty_cycle);

}
