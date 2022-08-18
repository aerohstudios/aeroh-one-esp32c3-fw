#include "iris.h"
#include "iris_record.h"
#include "iris_play.h"
#include "iris_repeater.h"

#include "logging.h"
#include "errors.h"
#include "storage.h"

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

#define NVS_FLASH_KEY_FROMATTER "rmt_cmd_%d"

void iris_play_from_memory(int memory_index) {
	// 1. Retrieve binary command from NVS Flash
	LOGI("==> 1. Running command from memory at index: %d", memory_index);
	void * retrieved_serialized_data = NULL;
	size_t retrieved_serialized_data_size = 0;
	char key[16];
	sprintf(key, NVS_FLASH_KEY_FROMATTER, memory_index);
	if (storage_get_blob(key, 0, &retrieved_serialized_data_size) == SUCCESS) {
		retrieved_serialized_data = malloc(retrieved_serialized_data_size);

		if (storage_get_blob(key, retrieved_serialized_data, &retrieved_serialized_data_size) == SUCCESS) {
			LOGI("Got %d bytes at %p", retrieved_serialized_data_size, retrieved_serialized_data);

			// 2. Deserialize Binary command to rmt_uint32_t for RMT Peripheral
			LOGI("==> 2. Deserialize Binary command to rmt_uint32_t");
			unsigned int duty_cycle = 0;
			unsigned int frequency = 0;
			unsigned int length = 0;
			rmt_item32_t * rmt_items;
			deserialize_data_to_rmt_items(retrieved_serialized_data, &duty_cycle, &frequency, &length, &rmt_items);
			free(retrieved_serialized_data);

			// 3. Play the command
			LOGI("==> 3. Play IR command");
			iris_play_command(duty_cycle, frequency, length, rmt_items);
			free(rmt_items);

			LOGI("==> END. Done!");
		} else {
			LOGE("Failed to retrieve key %s from NVS Flash", key);
		}

	} else {
		LOGE("Failed to retrieve key %s from NVS Flash", key);
	}
}

void iris_record_into_memory(int memory_index) {
	// 1. Record
	LOGI("==> 1. Recording command into memory at index: %d", memory_index);

	ir_command_t ir_command;
	iris_record_command(&ir_command);

	if (ir_command.length > 0) {
		// 2. Serialize command to binary
		LOGI("==> 2. Got signals. Serializing to binary.");
		size_t data_size = 0;
		void * serialized_data = NULL;
		serialize_data_from_ir_command(&ir_command, &serialized_data, &data_size);
		free(ir_command.signal_pairs);

		// 3. Store binary command in NVS Flash
		LOGI("==> 3. Storing Command into NVS Flash");
		LOGI("Got size %d", data_size);
		char key[16];
		sprintf(key, NVS_FLASH_KEY_FROMATTER, memory_index);
		LOGI("Writing command to key: %s", key);

		error_t storage_set_blob_ret = storage_set_blob(key, serialized_data, data_size);
		if (storage_set_blob_ret == SUCCESS) {
			LOGI("Success!");
		} else {
			LOGI("Got error! %d", storage_set_blob_ret);
		}
		free(serialized_data);
		LOGI("==> END. Done!");
	} else {
		LOGI("==> END. No command was recorded");
	}
}
