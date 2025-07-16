#include "operations.h"
#include "../handle_pool.h"
#include "../system_info.h"
#include <stdio.h>
#include <string.h>

extern handle_pool_t g_pool;

int method_destroy(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)params;
    if (!handle_pool_is_valid(&g_pool, handle_id)) return -1;
    
    LLMHandle* handle = handle_pool_get(&g_pool, handle_id);
    if (!handle) return -1;
    
    printf("🧹 Destroying handle %u...\n", handle_id);
    
    int ret = rkllm_destroy(*handle);
    if (ret != 0) {
        printf("❌ Failed to destroy RKLLM handle: %d\n", ret);
        return ret;
    }
    
    handle_pool_destroy(&g_pool, handle_id);
    
    // Force memory cleanup after destroying model
    system_force_gc();
    
    printf("✅ Handle %u destroyed and memory cleaned\n", handle_id);
    snprintf(result, result_size, "{\"status\":\"destroyed\"}");
    return 0;
}

int method_status(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)params;
    if (!handle_pool_is_valid(&g_pool, handle_id)) {
        snprintf(result, result_size, "{\"error\":\"Handle not found\"}");
        return -1;
    }
    
    size_t memory = handle_pool_get_memory_usage(&g_pool, handle_id);
    snprintf(result, result_size, "{\"handle_id\":%u,\"memory_usage\":%zu}", handle_id, memory);
    return 0;
}
