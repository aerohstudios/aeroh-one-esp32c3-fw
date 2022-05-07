/**
 * Firmware Version
 * ================
 * Good practice to update this on every new release
 * Should be sent with every request header to the Aeroh Cloud Servers
 **/

static const int AEROH_ONE_ESP32_C3_CORE_FW_VERSION_MAJOR = 1;
static const int AEROH_ONE_ESP32_C3_CORE_FW_VERSION_MINOR = 0;
static const int AEROH_ONE_ESP32_C3_CORE_FW_VERSION_PATCH = 0;

char * get_fw_version_string();
