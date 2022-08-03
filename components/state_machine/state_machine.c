/**
 * State Machine is responsible to managing state of the running operating
 * system.
 *
 * Documentation on state transitions can be found here:
 * https://app.diagrams.net/#G1WIsIk79gzEeN0bb6eXcFvSsARDPWS1F2
 *
 **/
#include <stdbool.h>
#include <stdint.h>

#include "errors.h"
#include "logging.h"
#include "storage.h"
#include "bluetooth_service.h"
#include "wifi_service.h"
#include "cloud.h"
#include "iris.h"

#include "state_machine.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MACHINE_STATE_KEY   "machine_state"

static int32_t machine_state = MACHINE_STATE_EMPTY;

void initialize_or_get_current_state();
error_t get_state_machine_state();

void run_current_state_callback(void) {
    switch(machine_state) {
        case MACHINE_STATE_NEW:
            initialize_bluetooth(); // TODO add failure condition
            break;
        case MACHINE_STATE_PROVISIONING_MQTT_CONNECTING:
            initialize_wifi();
            connect_to_wifi();
            initialize_spi_bus();
            add_device_to_spi_bus();
            initialize_mqtt_client();
            subscribe_to_aws_iot();
            break;
        default:
            LOGE("State callback not implimented for state: %d", machine_state);
    }
}

void reset_system_state_on_startup() {
    initialize_or_get_current_state();

    int32_t current_state = get_current_state_from_ram();

    // provisioning phase
    if (current_state == MACHINE_STATE_EMPTY ||
        (current_state >= MACHINE_STATE_PROVISIONING_BT_CONNECTED
         && current_state <= MACHINE_STATE_PROVISIONING_WIFI_CONNECTED_BT_TRANSFER)) {
        // initialization complete
        LOGI("Changing current state from %d to %d", current_state, MACHINE_STATE_NEW);

        set_state_machine_state(MACHINE_STATE_NEW);
    }

    // startup phase
    if (current_state >= MACHINE_STATE_STARTUP_WIFI_CONNECTING &&
        current_state <= MACHINE_STATE_STARTUP_MQTT_CONNECTED) {

        LOGI("Changing current state from %d to %d",
             current_state, MACHINE_STATE_PROVISIONING_MQTT_CONNECTING);

        set_state_machine_state(MACHINE_STATE_PROVISIONING_MQTT_CONNECTING);
    }
}


error_t start_state_machine() {
    LOGI("Starting State Machine");
    reset_system_state_on_startup();

    LOGI("Current Machine State: %d", machine_state);
    run_current_state_callback();
    return SUCCESS;
}

void initialize_or_get_current_state() {
    error_t get_state_err = get_state_machine_state();
    if (get_state_err == SUCCESS) {
        // we have the state in static variable machine_state
    } else if (get_state_err == STORAGE_KEY_NOT_FOUND) {
        error_t set_state_err = storage_set_int(MACHINE_STATE_KEY, MACHINE_STATE_NEW);
        if (set_state_err != SUCCESS) {
            LOGE("Failed to set initial machine state!");
            // TODO: Report Failure
        }
    } else {
        LOGE("Failed to get machine state!");
        // TODO: Report Failure
    }
}

int get_current_state_from_ram() {
    return machine_state;
}

error_t get_state_machine_state() {
    LOGV("Getting Machine State");
    return storage_get_int(MACHINE_STATE_KEY, &machine_state);
}

error_t set_state_machine_state(const int32_t new_machine_state) {
    LOGI("Setting Machine State to %d", new_machine_state);
    if (storage_set_int(MACHINE_STATE_KEY, new_machine_state) == SUCCESS) {
      machine_state = new_machine_state;
      return SUCCESS;
    } else {
      return FAILED;
    }
}
