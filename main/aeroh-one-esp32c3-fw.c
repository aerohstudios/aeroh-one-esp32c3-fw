#include <stdio.h>

#include "logging.h"
#include "errors.h"
#include "version.h"
#include "storage.h"
#include "state_machine.h"
#include "status_led.h"
#include "reset_button.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    LOGI("v%s Started.", get_fw_version_string());
    LOGI("FreeRTOS Kernel Version: %s", tskKERNEL_VERSION_NUMBER);

    if (initialize_flash_store() != SUCCESS) {
        LOGE("Couldn't initialize flash store!");
        display_startup_error();
    }

    xTaskCreate(vStatusLEDTask, "status_led", 4*1024, NULL, 1, NULL);
    xTaskCreate(vResetButtonTask, "reset_button", 4*1024, NULL, 1, NULL);

    start_state_machine();
}
