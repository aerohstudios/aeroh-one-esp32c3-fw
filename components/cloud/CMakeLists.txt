idf_component_register(SRCS "cloud.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "iris" "mqtt" "wifi_service" "storage" "errors" "logging")
