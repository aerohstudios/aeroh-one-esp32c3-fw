/**
 * Code in this file will be responsible to managing non-volatile
 * Data on the device.
 *
 * This includes:
 * - Device Initialization State
 * - Wifi Credentials
 * - MQTT Credentails
 *
 *  NVS Flash API Ref: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html#api-reference
 **/

#include <stdbool.h>
#include <stdint.h>

#include <esp_system.h>
#include "nvs_flash.h"

#include "errors.h"
#include "logging.h"

bool initialize_flash_store() {
    LOGI("Initializing flash store...");
    esp_err_t nvs_flash_init_err = nvs_flash_init();

    if (nvs_flash_init_err == ESP_OK) {
        LOGI("NVS flash initialized successfully!");
        return SUCCESS;
    } else {
        LOGE("Failed with NVS error code: %s", esp_err_to_name(nvs_flash_init_err));
        return FAILED;
    }
}

error_t storage_get_int(const char * key, int32_t * out_value) {
    nvs_handle_t nvs_handle;
    esp_err_t nvs_open_err = nvs_open(CONFIG_APP_ID, NVS_READWRITE, &nvs_handle);
    esp_err_t nvs_get_i32_err = ESP_FAIL;
    if (nvs_open_err == ESP_OK) {
        nvs_get_i32_err = nvs_get_i32(nvs_handle, key, out_value);
    }
    nvs_close(nvs_handle);

    if (nvs_open_err == ESP_OK && nvs_get_i32_err == ESP_OK) {
        return SUCCESS;
    } else if (nvs_open_err == ESP_OK && nvs_get_i32_err == ESP_ERR_NVS_NOT_FOUND) {
        return STORAGE_KEY_NOT_FOUND;
    } else {
        if (nvs_open_err != ESP_OK) {
            LOGE("nvs_open failed with error: %s", esp_err_to_name(nvs_open_err));
        } else {
            LOGE("nvs_get_i32 failed with error: %s", esp_err_to_name(nvs_get_i32_err));
        }
        return FAILED;
    }
}

error_t storage_set_int(const char * key, int32_t value) {
    nvs_handle_t nvs_handle;
    esp_err_t nvs_open_err = nvs_open(CONFIG_APP_ID, NVS_READWRITE, &nvs_handle);
    esp_err_t nvs_set_i32_err = ESP_FAIL;
    if (nvs_open_err == ESP_OK) {
        nvs_set_i32_err = nvs_set_i32(nvs_handle, key, value);
    }
    nvs_close(nvs_handle);

    if (nvs_open_err == ESP_OK && nvs_set_i32_err == ESP_OK) {
        return SUCCESS;
    } else if (nvs_open_err == ESP_OK && nvs_set_i32_err == ESP_ERR_NVS_NOT_ENOUGH_SPACE) {
        return STORAGE_OUT_OF_SPACE;
    } else {
        if (nvs_open_err != ESP_OK) {
            LOGE("nvs_open failed with error: %s", esp_err_to_name(nvs_open_err));
        } else {
            LOGE("nvs_set_i32 failed with error: %s", esp_err_to_name(nvs_set_i32_err));
        }
        return FAILED;
    }
}
