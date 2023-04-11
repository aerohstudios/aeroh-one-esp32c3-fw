idf_component_register(SRCS "reset_button.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "status_led" "storage" "errors" "logging" "state_machine")
