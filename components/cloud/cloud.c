#include <stddef.h>

#include "cJSON.h"

#include "mqtt_client.h"
#include "esp_tls.h"
#include "driver/rmt.h"

#include "logging.h"
#include "errors.h"
#include "storage.h"
#include "wifi_service.h"
#include "iris.h"

#include "cloud.h"

static char * root_ca = NULL;
static char * client_crt = NULL;
static char * client_key = NULL;
static char * client_id = NULL;
static char * mqtt_uri = NULL;

static bool mqtt_initialized = false;

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
            int cmd_len = cJSON_GetArraySize(root);
            LOGI("Got cmd_len %d", cmd_len);
            // [0] cmd_name
            char * cmd_name = "";
            if (cmd_len > 0) {
                cmd_name = cJSON_GetArrayItem(root, 0)->valuestring;
                LOGI("Got cmd_name = %s", cmd_name);
            }

            // [1] action_type
            char * action_type = "";
            if (cmd_len > 1) {
                action_type = cJSON_GetArrayItem(root, 1)->valuestring;
            }

            // [2] action_value
            char * action_value = "";
            if (cmd_len > 2) {
                action_value = cJSON_GetArrayItem(root, 2)->valuestring;
            }

            if (strcmp(cmd_name, "power") == 0 &&
                    strcmp(action_type, "toggle") == 0) {
                LOGI("Going to toggle power");
                iris_play_from_memory(0);
            } else if (strcmp(cmd_name, "speed") == 0 &&
                    strcmp(action_type, "change") == 0) {
                LOGI("Going to change speed");
                iris_play_from_memory(1);
            } else if (strcmp(cmd_name, "record") == 0 &&
                    strcmp(action_type, "power") == 0 &&
			        strcmp(action_value, "toggle") == 0) {
                LOGI("Going to change speed");
                iris_record_into_memory(0);
            } else if (strcmp(cmd_name, "record") == 0 &&
                    strcmp(action_type, "speed") == 0 &&
			        strcmp(action_value, "change") == 0) {
                LOGI("Going to change speed");
                iris_record_into_memory(1);
            } else {
                LOGE("Cannot understand command: \"%s\"; action_type: \"%s\"; action_value: \"%s\";",
                        cmd_name, action_type, action_value);
            }

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
