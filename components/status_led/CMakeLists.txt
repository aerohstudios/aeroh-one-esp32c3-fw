idf_component_register(SRCS "status_led.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "led_strip" "errors" "logging" "state_machine")
