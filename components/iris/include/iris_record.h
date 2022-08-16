/*
 * iris_record.h
 *
 *  Created on: Aug 16, 2022
 *      Author: shiv
 */
#ifndef COMPONENTS_IRIS_INCLUDE_IRIS_RECORD_H_
#define COMPONENTS_IRIS_INCLUDE_IRIS_RECORD_H_

#include "iris_typedefs.h"
#include "driver/gpio.h"

void initialize_for_record(gpio_num_t);
void cleanup_from_record(gpio_num_t);
void record_for(int, ir_command_t *);

#endif /* COMPONENTS_IRIS_INCLUDE_IRIS_RECORD_H_ */
