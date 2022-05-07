/**
 * Only helper methods here.
 * Version numbers can be changed in include/version.h file
 **/

#include <stdio.h>
#include "version.h"

char * get_fw_version_string() {
    static char version_string[16];
    sprintf(version_string, "%d.%d.%d",
        AEROH_ONE_ESP32_C3_CORE_FW_VERSION_MAJOR,
        AEROH_ONE_ESP32_C3_CORE_FW_VERSION_MINOR,
        AEROH_ONE_ESP32_C3_CORE_FW_VERSION_PATCH);

    return version_string;
}
