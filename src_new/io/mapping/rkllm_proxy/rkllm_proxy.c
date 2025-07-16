#include "rkllm_proxy.h"
#include <string.h>
#include <stdio.h>

// Method mapping table (without "rkllm_" prefix)
static method_mapping_t method_table[] = {
    {"init", proxy_init},
    {"run", proxy_run},
    {"destroy", proxy_destroy},
    {"status", proxy_status},
    {"lora_init", proxy_lora_init},
    {"abort", proxy_abort},
    {"clear_kv_cache", proxy_clear_kv_cache},
    {NULL, NULL}
};

int rkllm_proxy_init(void) {
    // Initialize proxy subsystem
    return 0;
}

int rkllm_proxy_execute(const char* method, uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!method || !result || result_size == 0) {
        return -1;
    }
    
    // Find method in table
    for (int i = 0; method_table[i].method; i++) {
        if (strcmp(method_table[i].method, method) == 0) {
            return method_table[i].func(handle_id, params, result, result_size);
        }
    }
    
    // Method not found
    snprintf(result, result_size, "{\"error\":\"Unknown method: %s\"}", method);
    return -1;
}

void rkllm_proxy_shutdown(void) {
    // Cleanup proxy subsystem
}
