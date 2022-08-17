/*
 * iris_play.h
 *
 *  Created on: Aug 17, 2022
 *      Author: shiv
 */

#ifndef COMPONENTS_IRIS_INCLUDE_IRIS_PLAY_H_
#define COMPONENTS_IRIS_INCLUDE_IRIS_PLAY_H_


#include "driver/gpio.h"
#include "driver/rmt.h"

void initialize_rmt_driver(gpio_num_t, unsigned int, unsigned int);
void play_command(unsigned int, rmt_item32_t *);
void cleanup_rmt_driver(gpio_num_t);


#endif /* COMPONENTS_IRIS_INCLUDE_IRIS_PLAY_H_ */
