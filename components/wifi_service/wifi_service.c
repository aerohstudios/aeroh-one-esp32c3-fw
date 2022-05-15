#include <string.h>

#include "errors.h"
#include "logging.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"

#include "wifi_service.h"

#define WIFI_LIST_NUM   10

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
    /* wifi_mode_t mode; */

    LOGI("IP_EVENT %d\n", event_id);

    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
        LOGI("IP_EVENT_STA_GOT_IP\n");

        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        /* esp_wifi_get_mode(&mode); */

        /* esp_blufi_extra_info_t info; */
        /* memset(&info, 0, sizeof(esp_blufi_extra_info_t)); */
        /* memcpy(info.sta_bssid, gl_sta_bssid, 6); */
        /* info.sta_bssid_set = true; */
        /* info.sta_ssid = gl_sta_ssid; */
        /* info.sta_ssid_len = gl_sta_ssid_len; */
        /* if (ble_is_connected == true) { */
        /*     esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info); */
        /* } else { */
        /*     LOGI("BLUFI BLE is not connected yet\n"); */
        /* } */
        break;
    }
    default:
        LOGI("default ip %d\n", event_id);
        break;
    }
    return;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    wifi_event_sta_connected_t *event;
    /* wifi_mode_t mode; */

    LOGI("WIFI_EVENT %d\n", event_id);

    switch (event_id) {
    case WIFI_EVENT_STA_START:
        LOGI("WIFI_EVENT_STA_START\n");
        // TODO: Decouple Connect
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_CONNECTED:
        LOGI("WIFI_EVENT_STA_CONNECTED\n");
        gl_sta_connected = true;
        event = (wifi_event_sta_connected_t*) event_data;
        memcpy(gl_sta_bssid, event->bssid, 6);
        memcpy(gl_sta_ssid, event->ssid, event->ssid_len);
        gl_sta_ssid_len = event->ssid_len;
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        LOGI("WIFI_EVENT_STA_DISCONNECTED\n");
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        wifi_event_sta_disconnected_t* event_ = (wifi_event_sta_disconnected_t*) event_data;
        printf("reason: %d\n", event_->reason);
        gl_sta_connected = false;
        memset(gl_sta_ssid, 0, 32);
        memset(gl_sta_bssid, 0, 6);
        gl_sta_ssid_len = 0;
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    case WIFI_EVENT_AP_START:
        // no-use
        /* Not handle currently */

        /* LOGI("WIFI_EVENT_AP_START\n"); */
        /* esp_wifi_get_mode(&mode); */

        /* /\* TODO: get config or information of softap, then set to report extra_info *\/ */
        /* if (ble_is_connected == true) { */
        /*     if (gl_sta_connected) { */
        /*         esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, NULL); */
        /*     } else { */
        /*         esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, 0, NULL); */
        /*     } */
        /* } else { */
        /*     LOGI("BLUFI BLE is not connected yet\n"); */
        /* } */

        break;
    case WIFI_EVENT_SCAN_DONE: {
        // no-use
        /* Not handle currently */

        /* LOGI("WIFI_EVENT_SCAN_DONE\n"); */
        /* uint16_t apCount = 0; */
        /* esp_wifi_scan_get_ap_num(&apCount); */
        /* if (apCount == 0) { */
        /*     LOGI("Nothing AP found"); */
        /*     break; */
        /* } */
        /* wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount); */
        /* if (!ap_list) { */
        /*     LOGE("malloc error, ap_list is NULL"); */
        /*     break; */
        /* } */
        /* ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_list)); */
        /* esp_blufi_ap_record_t * blufi_ap_list = (esp_blufi_ap_record_t *)malloc(apCount * sizeof(esp_blufi_ap_record_t)); */
        /* if (!blufi_ap_list) { */
        /*     if (ap_list) { */
        /*         free(ap_list); */
        /*     } */
        /*     LOGE("malloc error, blufi_ap_list is NULL"); */
        /*     break; */
        /* } */
        /* for (int i = 0; i < apCount; ++i) */
        /* { */
        /*     blufi_ap_list[i].rssi = ap_list[i].rssi; */
        /*     memcpy(blufi_ap_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid)); */
        /* } */

        /* if (ble_is_connected == true) { */
        /*     esp_blufi_send_wifi_list(apCount, blufi_ap_list); */
        /* } else { */
        /*     LOGI("BLUFI BLE is not connected yet\n"); */
        /* } */

        /* esp_wifi_scan_stop(); */
        /* free(ap_list); */
        /* free(blufi_ap_list); */

        break;
    }
    default:
        LOGI("default wifi %d\n", event_id);
        break;
    }
    return;
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
    //ESP_ERROR_CHECK( esp_wifi_start() );
}
