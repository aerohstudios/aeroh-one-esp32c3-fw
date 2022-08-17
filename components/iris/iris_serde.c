/*
 * iris_serde.c
 *
 *  Created on: Aug 16, 2022
 *      Author: shiv
 */

#include "stdlib.h"

#include "iris_typedefs.h"

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

void serialize_data_from_ir_command(ir_command_t * ir_command, void ** binary_data, int * data_size) {
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

void deserialize_data_to_rmt_items(void * binary_data, int num_bytes) {
	int size_of_unsigned_int = sizeof(unsigned int);
	int size_of_char = sizeof(char);

	char * ptr = (char *) binary_data;
	unsigned int idx = 0;

	unsigned int version = 0;
	convert_chars_to_uint(ptr+idx, &version);
	idx += size_of_unsigned_int;
	LOGI("Found Version %d", (int) version);

	unsigned int duty_cycle = 0;
	char duty_cycle_key = (char) *(ptr+idx);
	if (duty_cycle_key == 'd') {
		idx += size_of_char;

		convert_chars_to_uint(ptr+idx, &duty_cycle);
		idx += size_of_unsigned_int;

		LOGI("Duty Cycle %d", (int) duty_cycle);
	}

	unsigned int frequency = 0;
	char frequency_key = (char) *(ptr+idx);
	if (frequency_key == 'f') {
		idx += size_of_char;

		convert_chars_to_uint(ptr+idx, &frequency);
		idx += size_of_unsigned_int;

		LOGI("Frequency %d", (int) frequency);
	}

	unsigned int length = 0;
	char length_key = (char) *(ptr+idx);
	if (length_key == 'l') {
		idx += size_of_char;

		convert_chars_to_uint(ptr+idx, &length);
		idx += size_of_unsigned_int;

		LOGI("Length %d", (int) length);
	}

	char signal_pair_key = (char) *(ptr+idx);
	idx += size_of_char;

	if (signal_pair_key == 's') {
		for (int signal_pair_idx = 0; signal_pair_idx < length; signal_pair_idx++) {
			unsigned int high_time = 0;
			convert_chars_to_uint(ptr+idx, &high_time);
			idx += size_of_unsigned_int;

			unsigned int low_time = 0;
			convert_chars_to_uint(ptr+idx, &low_time);
			idx += size_of_unsigned_int;

			LOGI("signal_pair_idx: %d", signal_pair_idx);
			LOGI("high_time: %d", (int) high_time);
			LOGI("low_time: %d", (int) low_time);
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
