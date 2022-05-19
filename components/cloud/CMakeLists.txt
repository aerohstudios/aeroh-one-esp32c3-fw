idf_component_register(SRCS "cloud.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "mqtt" "wifi_service" "storage" "errors" "logging")
