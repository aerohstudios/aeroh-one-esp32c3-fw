idf_component_register(SRCS "cloud.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp_http_client" "esp_https_ota" "iris" "mqtt" "wifi_service" "storage" "errors" "logging")
