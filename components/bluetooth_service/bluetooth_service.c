#include "errors.h"
#include "logging.h"

#include "esp_bt.h"
#include "esp_blufi.h"

error_t initialize_bluetooth(void) {
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

    return SUCCESS;
}
