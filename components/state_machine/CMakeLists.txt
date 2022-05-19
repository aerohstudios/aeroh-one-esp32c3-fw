idf_component_register(SRCS "state_machine.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "cloud" "wifi_service" "errors" "logging" "storage" "bluetooth_service")
