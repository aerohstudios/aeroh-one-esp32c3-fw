idf_component_register(SRCS "bluetooth_service.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "bt" "blufi_adapter" "json" "wifi_service" "cloud" "state_machine" "errors" "logging")
