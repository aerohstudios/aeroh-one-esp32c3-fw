#include "iris.h"
#include "iris_record.h"

#include "logging.h"
#include "errors.h"

#include "driver/gpio.h"

#define GPIO_OUTPUT_PIN	((gpio_num_t) 10)
#define GPIO_INPUT_PIN	((gpio_num_t) 4)

void iris_record_command(ir_command_t * ir_command) {
	initialize_for_record(GPIO_INPUT_PIN);
	record_for(2000, ir_command); // record for 2 secs approx
	cleanup_from_record(GPIO_INPUT_PIN);
}
