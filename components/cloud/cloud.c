#include <stddef.h>

#include "cJSON.h"

#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "driver/rmt.h"

#include "logging.h"
#include "errors.h"
#include "storage.h"
#include "wifi_service.h"
#include "iris.h"

#include "cloud.h"

#include "ota_certificate.h"

static char * root_ca = NULL;
static char * client_crt = NULL;
static char * client_key = NULL;
static char * client_id = NULL;
static char * mqtt_uri = NULL;

static bool mqtt_initialized = false;

esp_err_t do_firmware_update(char * url);

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            LOGI("MQTT_EVENT_CONNECTED");
            char topic_name[128];
            sprintf(topic_name, "%s/commands", client_id);
            msg_id = esp_mqtt_client_subscribe(client, topic_name, 1);
            LOGI("sent subscribe successful, msg_id=%d", msg_id);
            LOGI("topic_name=%s", topic_name);
            break;
        case MQTT_EVENT_DATA:
            LOGI("MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            LOGI("Process Incoming data");

            cJSON *root = cJSON_Parse(event->data);

            LOGI("Created ROOT");

            char * request_id = "";
            cJSON *cJSONrequestId = cJSON_GetObjectItemCaseSensitive(root, "request_id");
            if (cJSON_IsString(cJSONrequestId) && cJSONrequestId->valuestring != NULL) {
                request_id = cJSONrequestId->valuestring;
                LOGI("Got Request ID: %s", request_id);
            }

            char *original_request_token = "";
            cJSON *cJSONoriginalRequestToken = cJSON_GetObjectItemCaseSensitive(root, "original_request_token");
            if (cJSON_IsString(cJSONoriginalRequestToken) && cJSONoriginalRequestToken->valuestring != NULL) {
                original_request_token = cJSONoriginalRequestToken->valuestring;
                LOGI("Got Original Request Token: %s", original_request_token);
            }

            char * command = "";
            cJSON *cJSONcommand = cJSON_GetObjectItemCaseSensitive(root, "command");
            if (cJSON_IsString(cJSONcommand) && cJSONcommand->valuestring != NULL) {
                command = cJSONcommand->valuestring;
                LOGI("Got Request ID: %s", command);
            }

            char * action_type = "";
            cJSON *cJSONactionType = cJSON_GetObjectItemCaseSensitive(root, "action_type");
            if (cJSON_IsString(cJSONactionType) && cJSONactionType->valuestring != NULL) {
                action_type = cJSONactionType->valuestring;
                LOGI("Got Request ID: %s", action_type);
            }

            char * action_value = "";
            cJSON *cJSONactionValue = cJSON_GetObjectItemCaseSensitive(root, "action_value");
            if (cJSON_IsString(cJSONactionValue) && cJSONactionValue->valuestring != NULL) {
                action_value = cJSONactionValue->valuestring;
                LOGI("Got Request ID: %s", action_value);
            }

            char response_topic_name[128];
            sprintf(response_topic_name, "%s/responses", client_id);

            cJSON *response = cJSON_CreateObject();
            cJSON_AddStringToObject(response, "requestId", request_id);
            cJSON_AddStringToObject(response, "original_request_token", original_request_token);

            if (strcmp(command, "power") == 0 &&
                    strcmp(action_type, "toggle") == 0) {
                LOGI("Going to toggle power");
                iris_play_from_memory(0);
                cJSON_AddStringToObject(response, "status", "success");
            } else if (strcmp(command, "speed") == 0 &&
                    strcmp(action_type, "change") == 0) {
                LOGI("Going to change speed");
                iris_play_from_memory(1);
                cJSON_AddStringToObject(response, "status", "success");
            } else if (strcmp(command, "record") == 0 &&
                    strcmp(action_type, "power") == 0 &&
			        strcmp(action_value, "toggle") == 0) {
                LOGI("Going to change speed");
                iris_record_into_memory(0);
                cJSON_AddStringToObject(response, "status", "success");
            } else if (strcmp(command, "record") == 0 &&
                    strcmp(action_type, "speed") == 0 &&
			        strcmp(action_value, "change") == 0) {
                LOGI("Going to change speed");
                iris_record_into_memory(1);
                cJSON_AddStringToObject(response, "status", "success");
            } else if (strcmp(command, "firmware") == 0 &&
                    strcmp(action_type, "version") == 0) {

                cJSON_AddStringToObject(response, "version", CONFIG_FIRMWARE_VERSION);
                cJSON_AddStringToObject(response, "status", "success");
            } else if (strcmp(command, "firmware") == 0 &&
                    strcmp(action_type, "update") == 0) {

                char *firmware_url = "";
                cJSON *cJSONfirmwareUrl = cJSON_GetObjectItemCaseSensitive(root, "firmware_url");
                if (cJSON_IsString(cJSONfirmwareUrl) && cJSONfirmwareUrl->valuestring != NULL) {
                    firmware_url = cJSONfirmwareUrl->valuestring;
                    LOGI("Got : %s", firmware_url);
                }

                cJSON_AddStringToObject(response, "action_type", "downloading");
                cJSON_AddStringToObject(response, "status", "success");
                char * response_str = cJSON_Print(response);
                esp_mqtt_client_publish(client, response_topic_name, response_str, strlen(response_str), 1, 0);
                do_firmware_update(firmware_url);

                break;
            } else {
                cJSON_AddStringToObject(response, "status", "failed");
                cJSON_AddStringToObject(response, "error", "cannot understand request");

                LOGE("Cannot understand command: \"%s\"; action_type: \"%s\"; action_value: \"%s\";",
                        command, action_type, action_value);
            }

            char * response_str = cJSON_Print(response);
            esp_mqtt_client_publish(client, response_topic_name, response_str, strlen(response_str), 1, 0);

            break;
        case MQTT_EVENT_SUBSCRIBED:
            LOGI("MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            LOGI("MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            LOGI("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            LOGI("MQTT_EVENT_DISCONNECTED");
            LOGI("Error Type: %d, %s", event->error_handle->error_type, esp_err_to_name(event->error_handle->error_type));
            break;
        case MQTT_EVENT_ERROR:
            LOGI("MQTT_EVENT_ERROR");
            break;
        default:
            LOGI("Other event id: %d", event->event_id);
            break;
    }
    return ESP_OK;
}

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_https_ota.html
esp_err_t do_firmware_update(char * url)
{
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = (char *) ota_certificate_crt
    };
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        esp_restart();
    } else {
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void start_mqtt_client()
{
    size_t client_crt_len = 0;
    size_t client_key_len = 0;
    size_t client_id_len = 0;
    size_t mqtt_uri_len = 0;

    storage_get_str("certificate_pem", 0, &client_crt_len);
    storage_get_str("certificate_pri", 0, &client_key_len);
    storage_get_str("thing_name", 0, &client_id_len);
    storage_get_str("mqtt_uri", 0, &mqtt_uri_len);

    if (client_crt != NULL) {
        free(client_crt);
    }
    if (client_key != NULL) {
        free(client_key);
    }
    if (client_id != NULL) {
        free(client_id);
    }
    if (mqtt_uri != NULL) {
        free(mqtt_uri);
    }

    client_crt = malloc(sizeof(char) * client_crt_len);
    client_key = malloc(sizeof(char) * client_key_len);
    client_id = malloc(sizeof(char) * client_id_len);
    mqtt_uri = malloc(sizeof(char) * mqtt_uri_len);

    storage_get_str("certificate_pem", client_crt, &client_crt_len);
    storage_get_str("certificate_pri", client_key, &client_key_len);
    storage_get_str("thing_name", client_id, &client_id_len);
    storage_get_str("mqtt_uri", mqtt_uri, &mqtt_uri_len);

    LOGI("Got certificate_pem!");
    LOGI("Got certificate_private_key!");
    LOGI("Got thing_name!");
    LOGI("Got mqtt_uri!");

    const esp_mqtt_client_config_t mqtt_cfg = {
        .client_id = (const char *) client_id,
        .uri = (const char *) mqtt_uri,
        .event_handle = mqtt_event_handler,
        .client_cert_pem = (const char *) client_crt,
        .client_key_pem = (const char *) client_key,
        .use_global_ca_store = true
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void initialize_mqtt_client() {
    LOGI("Initializing MQTT Client");

    size_t root_ca_len = 0;

    if (storage_get_str("root_ca", 0, &root_ca_len) == SUCCESS) {
        if (root_ca != NULL) {
            free(root_ca);
        }
        root_ca = malloc(sizeof(char) * root_ca_len);

        storage_get_str("root_ca", root_ca, &root_ca_len);

        LOGI("Got root_ca!");

        ESP_ERROR_CHECK(esp_tls_init_global_ca_store());
        ESP_ERROR_CHECK(esp_tls_set_global_ca_store((const uint8_t *) root_ca, root_ca_len));

        LOGI("MQTT Client Initialized!");
        mqtt_initialized = true;
    } else {
        LOGI("Failed to initialize MQTT Client, will try later!");
    }
}

void subscribe_to_aws_iot()
{
    while (!is_wifi_connected()) {
        LOGI("Waiting for Wifi to be connected");
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (!mqtt_initialized) {
        initialize_mqtt_client();
    }

    LOGI("Connecting to AWS");
    start_mqtt_client();
}
