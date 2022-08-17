/*
 * iris_serde.h
 *
 *  Created on: Aug 16, 2022
 *      Author: shiv
 */

#ifndef COMPONENTS_IRIS_INCLUDE_IRIS_SERDE_H_
#define COMPONENTS_IRIS_INCLUDE_IRIS_SERDE_H_

#include "driver/rmt.h"

void serialize_data_from_ir_command(ir_command_t *, void **, size_t *);
void deserialize_data_to_rmt_items(void *, unsigned int *, unsigned int *, unsigned int *, rmt_item32_t **);


#endif /* COMPONENTS_IRIS_INCLUDE_IRIS_SERDE_H_ */
