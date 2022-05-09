idf_component_register(SRCS "bluetooth_service.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "bt" "blufi_adapter" "json" "errors" "logging")
