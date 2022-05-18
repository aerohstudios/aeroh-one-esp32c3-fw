#include "errors.h"
#include "logging.h"

#include "esp_bt.h"
#include "esp_blufi_api.h"
#include "blufi_adapter.h"
#include "esp_blufi.h"

#include "state_machine.h"
#include "wifi_service.h"

#include "cJSON.h"

static void aeroh_one_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);

static char * wifi_password = NULL;
static char * wifi_ssid = NULL;

static bool ble_is_connected = false;

void wifi_success_callback(uint8_t * bssid, uint8_t * ssid, int ssid_len) {
    LOGI("WIFI Success Callback Called");
    if (ble_is_connected) {
        esp_blufi_extra_info_t info;
        memset(&info, 0, sizeof(esp_blufi_extra_info_t));
        memcpy(info.sta_bssid, bssid, 6);
        info.sta_bssid_set = true;
        info.sta_ssid = ssid;
        info.sta_ssid_len = ssid_len;
        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
    }
}

void wifi_failure_callback(void) {
    LOGI("WIFI Failure Callback Called");
    if (ble_is_connected) {
        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
    }
}

static void aeroh_one_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        BLUFI_INFO("BLUFI init finish\n");

        esp_blufi_adv_start();
        break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        BLUFI_INFO("BLUFI deinit finish\n");
        break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
        BLUFI_INFO("BLUFI ble connect\n");
        ble_is_connected = true;
        esp_blufi_adv_stop();
        blufi_security_init();
        set_state_machine_state(MACHINE_STATE_PROVISIONING_BT_CONNECTED);
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        BLUFI_INFO("BLUFI ble disconnect\n");
        ble_is_connected = false;
        blufi_security_deinit();
        esp_blufi_adv_start();
        if (get_current_state_from_ram() == MACHINE_STATE_PROVISIONING_BT_CONNECTED) {
            // back to new state if provisioning isn't complete
            set_state_machine_state(MACHINE_STATE_NEW);
        }
        break;
    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        BLUFI_INFO("BLUFI requset wifi connect to AP\n");
        /* there is no wifi callback when the device has already connected to this wifi
        so disconnect wifi before connection.
        */
        connect_to_wifi(wifi_ssid, wifi_password, wifi_success_callback, wifi_failure_callback);
        break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
        /* Not handle currently */
        break;
    }
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        BLUFI_INFO("blufi close a gatt connection");
        esp_blufi_disconnect();
        break;
    case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
        /* TODO */
        break;
    case ESP_BLUFI_EVENT_RECV_STA_BSSID:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_STA_SSID:
        set_state_machine_state(MACHINE_STATE_PROVISIONING_BT_TRANSFER);
        if (wifi_ssid != NULL) {
            free(wifi_ssid);
            wifi_ssid = NULL;
        }

        wifi_ssid = malloc(sizeof(char) * (param->sta_ssid.ssid_len + 1));

        strncpy(wifi_ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        wifi_ssid[param->sta_ssid.ssid_len] = '\0';
        BLUFI_INFO("Recv STA SSID %s\n", wifi_ssid);
        break;
    case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        set_state_machine_state(MACHINE_STATE_PROVISIONING_BT_TRANSFER);
        if (wifi_password != NULL) {
            free(wifi_password);
            wifi_password = NULL;
        }

        wifi_password = malloc(sizeof(char) * (param->sta_passwd.passwd_len + 1));

        strncpy(wifi_password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
        wifi_password[param->sta_passwd.passwd_len] = '\0';
        BLUFI_INFO("Recv STA PASSWORD %s\n", wifi_password);
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_SSID:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST:{
        /* Not handle currently */
        break;
    }
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
        set_state_machine_state(MACHINE_STATE_PROVISIONING_WIFI_CONNECTED_BT_TRANSFER);

        BLUFI_INFO("Recv Custom Data 1: %d\n", param->custom_data.data_len);
        esp_log_buffer_hex("Custom Data 1: ", param->custom_data.data, param->custom_data.data_len);

        char *recv_msg = malloc(sizeof(char) * (param->custom_data.data_len + 1));
        memcpy(recv_msg, param->custom_data.data, param->custom_data.data_len);
        recv_msg[param->custom_data.data_len] = '\0';
        BLUFI_INFO("Custom Data 2: %s\n", recv_msg);

        cJSON *root = cJSON_Parse(recv_msg);

        if (cJSON_GetObjectItem(root, "certificate_pem")) {
            char *certificate_pem = cJSON_GetObjectItem(root, "certificate_pem")->valuestring;
            BLUFI_INFO("certificate_pem=%s", certificate_pem);
        }

        if (cJSON_GetObjectItem(root, "certificate_public_key")) {
            char *certificate_public_key = cJSON_GetObjectItem(root, "certificate_public_key")->valuestring;
            BLUFI_INFO("certificate_public_key=%s", certificate_public_key);
        }

        if (cJSON_GetObjectItem(root, "certificate_private_key")) {
            char *certificate_private_key = cJSON_GetObjectItem(root, "certificate_private_key")->valuestring;
            BLUFI_INFO("certificate_private_key=%s", certificate_private_key);
        }

        unsigned char reply_msg[6] = "Ack";
        esp_blufi_send_custom_data(reply_msg, 6);

        break;
    case ESP_BLUFI_EVENT_RECV_USERNAME:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_CA_CERT:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
        /* Not handle currently */
        break;
    case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
        /* Not handle currently */
        break;;
    case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
        /* Not handle currently */
        break;
    default:
        break;
    }
}

static esp_blufi_callbacks_t aeroh_one_blufi_callbacks = {
    .event_cb = aeroh_one_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

error_t initialize_bluetooth(void) {
    initialize_wifi();

    LOGI("Initializing BLE!");

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    error_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        LOGE("%s initialize bt controller failed: %s\n", __func__, esp_err_to_name(ret));
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        LOGE("%s enable bt controller failed: %s\n", __func__, esp_err_to_name(ret));
        return FAILED;
    }

    ret = esp_blufi_host_and_cb_init(&aeroh_one_blufi_callbacks);
    if (ret) {
        LOGE("%s initialise failed: %s\n", __func__, esp_err_to_name(ret));
        return FAILED;
    }

    LOGI("BLUFI VERSION %04x\n", esp_blufi_get_version());


    return SUCCESS;
}
