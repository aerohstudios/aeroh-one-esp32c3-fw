#include <string.h>

#include "errors.h"
#include "logging.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"

#include "wifi_service.h"

#define WIFI_LIST_NUM   10

#define DISCONNECT_LIMIT_FOR_FAILURE 10

#define CONNECT_RETRY_DELAY_IN_MS 200

static uint8_t retry_counter = 0;

void (* success_callback)(uint8_t *, uint8_t *, int) = NULL;

void (* failure_callback)(void) = NULL;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* store the station info for send back to phone */
static bool gl_sta_connected = false;
static uint8_t gl_sta_bssid[6];
static uint8_t gl_sta_ssid[32];
static int gl_sta_ssid_len;

static void ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
        LOGI("IP_EVENT_STA_GOT_IP\n");

        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

        success_callback(gl_sta_bssid, gl_sta_ssid, gl_sta_ssid_len);
        break;
    }
    default:
        break;
    }
    return;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{

    switch (event_id) {
    case WIFI_EVENT_STA_START:
        LOGI("WIFI_EVENT_STA_START\n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        LOGI("WIFI_EVENT_STA_CONNECTED\n");

        gl_sta_connected = true;

        wifi_event_sta_connected_t* conn_event = (wifi_event_sta_connected_t*) event_data;

        memcpy(gl_sta_bssid, conn_event->bssid, 6);
        memcpy(gl_sta_ssid, conn_event->ssid, conn_event->ssid_len);
        gl_sta_ssid_len = conn_event->ssid_len;

        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        LOGI("WIFI_EVENT_STA_DISCONNECTED\n");
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        gl_sta_connected = false;
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);

        wifi_event_sta_disconnected_t* dconn_event = (wifi_event_sta_disconnected_t*) event_data;
        LOGI("WIFI disconnection reason: %d\n", dconn_event->reason);

        memset(gl_sta_ssid, 0, 32);
        memset(gl_sta_bssid, 0, 6);
        gl_sta_ssid_len = 0;


        vTaskDelay(pdMS_TO_TICKS(CONNECT_RETRY_DELAY_IN_MS));
        if (retry_counter < DISCONNECT_LIMIT_FOR_FAILURE) {
            retry_counter += 1;
            LOGI("WIFI Retrying to connect, attempt no. %d", retry_counter);
        } else if (retry_counter == DISCONNECT_LIMIT_FOR_FAILURE) {
            // Report Failure
            failure_callback();

            retry_counter += 1;
            LOGI("WIFI Connection Failure, will continue to retry");
        } else {
            LOGI("WIFI Retrying to connect in perpetual mode");
        }
        esp_wifi_connect();

        break;
    case WIFI_EVENT_AP_START:
        /* Not handle currently */
        break;
    case WIFI_EVENT_SCAN_DONE: {
        /* Not handle currently */
        break;
    }
    default:
        break;
    }
    return;
}


void connect_to_wifi(char * wifi_ssid, char * wifi_password, void (* success_clbk)(uint8_t *, uint8_t *, int), void (* failure_clbk)(void)) {
    // reset retry counter
    retry_counter = 0;

    static wifi_config_t sta_config;

    uint8_t ssid_len = strlen(wifi_ssid);
    uint8_t password_len = strlen(wifi_password);

    strncpy((char *)sta_config.sta.ssid, wifi_ssid, ssid_len);
    strncpy((char *)sta_config.sta.password, wifi_password, password_len);

    sta_config.sta.ssid[ssid_len] = '\0';
    sta_config.sta.password[password_len] = '\0';

    esp_wifi_set_config(WIFI_IF_STA, &sta_config);

    success_callback = success_clbk;
    failure_callback = failure_clbk;

    if (gl_sta_connected) {
        esp_wifi_disconnect();
    }

    esp_wifi_connect();
}

void initialize_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}
