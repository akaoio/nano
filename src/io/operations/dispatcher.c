#include "operations.h"
#include <string.h>

static operation_t operations[] = {
    {"init", method_init},
    {"run", method_run},
    {"destroy", method_destroy},
    {"status", method_status},
    {"lora_init", method_lora_init},
    {NULL, NULL}
};

int execute_method(const char* method, uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!method || !result || result_size == 0) return -1;
    
    for (int i = 0; operations[i].method; i++) {
        if (strcmp(operations[i].method, method) == 0) {
            return operations[i].func(handle_id, params, result, result_size);
        }
    }
    return -1;
}
