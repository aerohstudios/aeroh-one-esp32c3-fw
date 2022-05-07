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

#include "state_machine.h"

#define MACHINE_STATE_KEY   "machine_state"

static int32_t machine_state = MACHINE_STATE_EMPTY;

void initialize_or_get_current_state();
error_t get_state_machine_state();
error_t set_state_machine_state(const int32_t new_machine_state);

error_t start_state_machine() {
    LOGI("Starting State Machine");
    initialize_or_get_current_state();
    LOGI("Current Machine State: %d", machine_state);
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
    return storage_set_int(MACHINE_STATE_KEY, new_machine_state);
}
