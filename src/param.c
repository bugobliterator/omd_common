#include "param.h"
#include "uavcan.h"

#define MAX_NUM_PARAMS 50

#define PARAM_STORAGE_SIZE 1024

MUTEX_DECL(param_mutex);

static uint8_t param_storage[PARAM_STORAGE_SIZE][2];
static const struct param_descriptor_header_s* param_descriptor_table[MAX_NUM_PARAMS];
static uint16_t num_params_registered;

void _param_register(struct param_descriptor_header_s* param_descriptor_header) {
    param_descriptor_table[num_params_registered] = param_descriptor_header;
    num_params_registered++;
}

void param_register(struct param_descriptor_header_s* param_descriptor_header) {
    chMtxLock(param_mutex);
    _param_register(param_descriptor_header);
    chMtxUnlock(param_mutex);
}

void param_print_table(void) {
    for (uint16_t i=0; i<num_params_registered; i++) {
        if (!param_descriptor_table[i]) {
            break;
        }

        uavcan_send_debug_logmessage(UAVCAN_LOGLEVEL_DEBUG, "param", param_descriptor_table[i]->name);
    }
}
