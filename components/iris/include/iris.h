#include "iris_typedefs.h"
#include "iris_serde.h"

void iris_start_repeater();
void iris_record_command(ir_command_t *);
void iris_play_command(unsigned int, unsigned int, unsigned int, rmt_item32_t *);
void iris_play_from_memory(int);
void iris_record_into_memory(int);
