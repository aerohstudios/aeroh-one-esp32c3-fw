#include <stddef.h>

#include "mqtt_client.h"
#include "esp_tls.h"

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


void int_to_chars(int d, char * higher, char * lower) {
	*lower = d & 0xff;
	*higher = (d >> 8) & 0xff;
}

void chars_to_int(char higher, char lower, int * e) {
	*e = ((higher << 8) & 0xff00) | (lower & 0xff);
}

void serialize_command(char * serialized_command, int * command_size) {
	char version = 0x1;
	char command_type = 'p';
	char repeat_key = 'r';
	int repeat_value = 8;
	int data_stream[12][2] = {
		{1301, 384},
        {1301, 384},

        {448, 1259},
        {1301, 384},

        {1301, 384},
        {448, 1259},

        {448, 1259},
        {448, 1259},

        {448, 1259},
        {1301, 384},

        {448, 1259},
        {448, 8128}
	};

	int idx = 0;
	serialized_command[idx++] = version;
	serialized_command[idx++] = command_type;
	serialized_command[idx++] = repeat_key;
	int_to_chars(repeat_value, &serialized_command[idx], &serialized_command[idx+1]);
	idx++;
	idx++;
	serialized_command[idx++] = 'd';
	serialized_command[idx++] = 'l';
	int_to_chars(24, &serialized_command[idx], &serialized_command[idx+1]);
	idx++;
	idx++;
	for (int i = 0; i < 12; i++) {
		serialized_command[idx++] = '1';
		int_to_chars(data_stream[i][0], &serialized_command[idx], &serialized_command[idx+1]);
		idx++;
		idx++;
		serialized_command[idx++] = '0';
		int_to_chars(data_stream[i][1], &serialized_command[idx], &serialized_command[idx+1]);
		idx++;
		idx++;
	}
	serialized_command[idx++] = 'e';
    *command_size = idx+1;
}

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

            char payload[128] = "";
            int payload_len = 0;
            serialize_command(&payload, &payload_len);

            initialize_spi_bus();
            add_device_to_spi_bus();
            send_data_to_iris((char *) &payload, payload_len);
            remove_device_from_spi_bus();
            free_spi_bus();

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
        LOGI("Failed to initalize MQTT Client, will try later!");
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
