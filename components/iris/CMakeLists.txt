idf_component_register(SRCS "iris.c" "iris_record.c" "iris_serde.c" "iris_play.c" "iris_repeater.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "errors" "logging")
