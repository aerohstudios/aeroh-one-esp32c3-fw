/*
 * iris_serde.c
 *
 *  Created on: Aug 16, 2022
 *      Author: shiv
 */

#include "stdlib.h"

#include "iris_typedefs.h"
#include "iris_serde.h"

#include "logging.h"
#include "errors.h"

#define ASCII_ETX '\3'

void convert_uint_to_chars(char * loc, int value) {
	*loc = (value & 0xff000000) >> 24;
	*(loc+1) = (value & 0x00ff0000) >> 16;
	*(loc+2) = (value & 0x0000ff00) >> 8;
	*(loc+3) = value & 0x000000ff;
}

void convert_chars_to_uint(char * loc, unsigned int * value) {
	*value = (unsigned int) ((*loc << 24) | (*(loc + 1) << 16) | (*(loc+2) << 8) | *(loc+3));
}

void serialize_data_from_ir_command(ir_command_t * ir_command, void ** binary_data, size_t * data_size) {
	int size_of_unsigned_int = sizeof(unsigned int);
	int size_of_char = sizeof(char);
	*data_size = (size_of_char * 4) + (size_of_unsigned_int*((ir_command->length*2)+4)) + 1;
	char * ptr = (char *) malloc(*data_size);
	unsigned int idx = 0;

	convert_uint_to_chars(ptr+idx, ir_command->version);
	idx += size_of_unsigned_int;

	*(ptr+idx) = 'd'; // duty cycle
	idx += size_of_char;

	convert_uint_to_chars(ptr+idx, ir_command->duty_cycle);
	idx += size_of_unsigned_int;

	*(ptr+idx) = 'f'; // frequency
	idx += size_of_char;

	convert_uint_to_chars(ptr+idx, ir_command->frequency);
	idx += size_of_unsigned_int;

	*(ptr+idx) = 'l'; // length
	idx += size_of_char;

	convert_uint_to_chars(ptr+idx, ir_command->length);
	idx += size_of_unsigned_int;

	*(ptr+idx) = 's'; // signal pairs
	idx += size_of_char;

	for (int signal_pair_idx = 0; signal_pair_idx < ir_command->length; signal_pair_idx++) {
		convert_uint_to_chars(ptr+idx, ir_command->signal_pairs[signal_pair_idx].high_time);
		idx += size_of_unsigned_int;

		convert_uint_to_chars(ptr+idx, ir_command->signal_pairs[signal_pair_idx].low_time);
		idx += size_of_unsigned_int;
	}

	*(ptr+idx) = ASCII_ETX;
	idx += 1;
	*binary_data = (void *) ptr;
	LOGI("Binary Data Location %p", *binary_data);
	LOGI("Binary Data Size: %d", *data_size);
}

void deserialize_data_to_rmt_items(void * binary_data, unsigned int * duty_cycle, unsigned int * frequency, unsigned int * length, rmt_item32_t ** rmt_items) {
	int size_of_unsigned_int = sizeof(unsigned int);
	int size_of_char = sizeof(char);

	char * ptr = (char *) binary_data;
	unsigned int idx = 0;

	unsigned int version = 0;
	convert_chars_to_uint(ptr+idx, &version);
	idx += size_of_unsigned_int;
	LOGI("Found Version %d", (int) version);

	char duty_cycle_key = (char) *(ptr+idx);
	if (duty_cycle_key == 'd') {
		idx += size_of_char;

		convert_chars_to_uint(ptr+idx, duty_cycle);
		idx += size_of_unsigned_int;

		LOGI("Duty Cycle %d", (int) *duty_cycle);
	}

	char frequency_key = (char) *(ptr+idx);
	if (frequency_key == 'f') {
		idx += size_of_char;

		convert_chars_to_uint(ptr+idx, frequency);
		idx += size_of_unsigned_int;

		LOGI("Frequency %d", (int) *frequency);
	}

	char length_key = (char) *(ptr+idx);
	if (length_key == 'l') {
		idx += size_of_char;

		convert_chars_to_uint(ptr+idx, length);
		idx += size_of_unsigned_int;

		LOGI("Length %d", (int) *length);
	}

	size_t malloc_size = sizeof(rmt_item32_t) * (*length);
	LOGI("Malloc size %d", (int) malloc_size);

	void * malloc_ptr;
	malloc_ptr = malloc(malloc_size);
	*rmt_items = malloc_ptr;

	char signal_pair_key = (char) *(ptr+idx);
	idx += size_of_char;

	if (signal_pair_key == 's') {
		for (int signal_pair_idx = 0; signal_pair_idx < (*length); signal_pair_idx++) {
			rmt_item32_t * current_rmt_item;
			current_rmt_item = (rmt_item32_t *) ( malloc_ptr + (signal_pair_idx*sizeof(rmt_item32_t)) );


			unsigned int high_time = 0;
			convert_chars_to_uint(ptr+idx, &high_time);

			(*current_rmt_item).duration0 = (int) high_time;
			(*current_rmt_item).level0 = 1;

			idx += size_of_unsigned_int;


			unsigned int low_time = 0;
			convert_chars_to_uint(ptr+idx, &low_time);

			(*current_rmt_item).duration1 = (int) low_time;
			(*current_rmt_item).level1 = 0;

			idx += size_of_unsigned_int;

			//printf("idx: %d; high: %d; low: %d; loc: %p;\n", signal_pair_idx, high_time, low_time, current_rmt_item);
		}
	}

	char eof = (char) *(ptr+idx);
	idx += 1;

	if (eof == ASCII_ETX) {
		LOGI("Found EOF");
	} else {
		LOGI("Couldn't find EOF");
	}
}
