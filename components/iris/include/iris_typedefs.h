/*
 * iris_typedefs.h
 *
 *  Created on: Aug 16, 2022
 *      Author: shiv
 */

#ifndef COMPONENTS_IRIS_INCLUDE_IRIS_TYPEDEFS_H_
#define COMPONENTS_IRIS_INCLUDE_IRIS_TYPEDEFS_H_

typedef struct {
	unsigned int high_time;
	unsigned int low_time;
} signal_pair_t;

typedef struct {
	unsigned int version;
	unsigned int duty_cycle;
	unsigned int frequency;
	unsigned int length;
	signal_pair_t * signal_pairs;
} ir_command_t;

#endif /* COMPONENTS_IRIS_INCLUDE_IRIS_TYPEDEFS_H_ */
