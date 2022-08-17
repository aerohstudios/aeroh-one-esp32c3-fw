/*
 * ir_repeater.h
 *
 *  Created on: Aug 17, 2022
 *      Author: shiv
 */

#ifndef COMPONENTS_IRIS_INCLUDE_IRIS_REPEATER_H_
#define COMPONENTS_IRIS_INCLUDE_IRIS_REPEATER_H_

#include "driver/gpio.h"

void initialize_repeater(gpio_num_t gpio_input_pin, gpio_num_t gpio_output_pin);
void clear_repeater(gpio_num_t gpio_input_pin, gpio_num_t gpio_output_pin);


#endif /* COMPONENTS_IRIS_INCLUDE_IRIS_REPEATER_H_ */
