#include "iris.h"
#include "iris_record.h"
#include "iris_play.h"
#include "iris_repeater.h"

#include "logging.h"
#include "errors.h"

#include "driver/gpio.h"

#define GPIO_OUTPUT_PIN	((gpio_num_t) 10)
#define GPIO_INPUT_PIN	((gpio_num_t) 4)

void iris_start_repeater() {
	initialize_repeater(GPIO_INPUT_PIN, GPIO_OUTPUT_PIN);
}

void iris_record_command(ir_command_t * ir_command) {
	clear_repeater(GPIO_INPUT_PIN, GPIO_OUTPUT_PIN);
	initialize_for_record(GPIO_INPUT_PIN);
	record_for(2000, ir_command); // record for 2 secs approx
	cleanup_from_record(GPIO_INPUT_PIN);
	initialize_repeater(GPIO_INPUT_PIN, GPIO_OUTPUT_PIN);
}

void iris_play_command(unsigned int duty_cycle, unsigned int frequency, unsigned int length, rmt_item32_t * rmt_items) {
	clear_repeater(GPIO_INPUT_PIN, GPIO_OUTPUT_PIN);
	initialize_rmt_driver(GPIO_OUTPUT_PIN, duty_cycle, frequency);
	play_command(length, rmt_items);
	cleanup_rmt_driver(GPIO_OUTPUT_PIN);
	initialize_repeater(GPIO_INPUT_PIN, GPIO_OUTPUT_PIN);
}
