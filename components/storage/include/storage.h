#include <stdbool.h>
#include <stddef.h>

#include "errors.h"

error_t initialize_flash_store();
error_t storage_get_int(const char * key, int32_t * out_value);
error_t storage_set_int(const char * key, int32_t value);

error_t storage_get_str(const char * key, char * out_value, size_t * length);
error_t storage_set_str(const char * key, const char * value);

error_t storage_get_blob(const char * key, void * out_value, size_t * length);
error_t storage_set_blob(const char * key, const void * value, size_t length);
