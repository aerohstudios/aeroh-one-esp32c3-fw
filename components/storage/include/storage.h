#include <stdbool.h>

#include "errors.h"

error_t initialize_flash_store();
error_t storage_get_int(const char * key, int32_t * out_value);
error_t storage_set_int(const char * key, int32_t value);
