#pragma once
/**
 * ESP's Logging Capability is defined in esp_log.h file:
 * https://github.com/espressif/esp-idf/blob/master/components/log/include/esp_log.h#L331
 *
 * We have abstracted logging functionality to extend esp_log's functionality to add
 * some useful information for debugging purposes, and without increasing the complexity
 * on the caller.
 **/

#include <esp_log.h>

#define LOGE(format, ...) ESP_LOGE(CONFIG_APP_ID, format, ##__VA_ARGS__)
#define LOGW(format, ...) ESP_LOGW(CONFIG_APP_ID, format, ##__VA_ARGS__)
#define LOGI(format, ...) ESP_LOGI(CONFIG_APP_ID, format, ##__VA_ARGS__)
#define LOGD(format, ...) ESP_LOGD(CONFIG_APP_ID, format, ##__VA_ARGS__)
#define LOGV(format, ...) ESP_LOGV(CONFIG_APP_ID, format, ##__VA_ARGS__)
