#include "rkllm_proxy.h"
#include "../handle_pool/handle_pool.h"
#include <string.h>
#include <stdio.h>

// Global handle pool
extern handle_pool_t g_handle_pool;

int proxy_init(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!params || !result || result_size == 0) {
        return -1;
    }
    
    // Extract model_path from JSON params
    char model_path[256];
    if (!json_get_string(params, "model_path", model_path, sizeof(model_path))) {
        snprintf(result, result_size, "{\"error\":\"Missing model_path parameter\"}");
        return -1;
    }
    
    // Create handle in pool
    uint32_t new_handle_id = handle_pool_create(&g_handle_pool, model_path);
    if (new_handle_id == 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to create handle\"}");
        return -1;
    }
    
    // Return success with handle_id
    snprintf(result, result_size, "{\"handle_id\":%u,\"status\":\"success\"}", new_handle_id);
    return 0;
}

int proxy_run(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!params || !result || result_size == 0) {
        return -1;
    }
    
    // Get handle from pool
    LLMHandle* handle = handle_pool_get(&g_handle_pool, handle_id);
    if (!handle) {
        snprintf(result, result_size, "{\"error\":\"Invalid handle_id: %u\"}", handle_id);
        return -1;
    }
    
    // Extract prompt from JSON params
    char prompt[1024];
    if (!json_get_string(params, "prompt", prompt, sizeof(prompt))) {
        snprintf(result, result_size, "{\"error\":\"Missing prompt parameter\"}");
        return -1;
    }
    
    // TODO: Implement actual RKLLM run call
    // For now, return mock response
    snprintf(result, result_size, "{\"response\":\"Mock response for prompt: %s\"}", prompt);
    return 0;
}

int proxy_destroy(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)params; // Unused parameter
    
    if (!result || result_size == 0) {
        return -1;
    }
    
    // Destroy handle in pool
    int ret = handle_pool_destroy(&g_handle_pool, handle_id);
    if (ret != 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to destroy handle %u\"}", handle_id);
        return -1;
    }
    
    snprintf(result, result_size, "{\"status\":\"destroyed\"}");
    return 0;
}

int proxy_status(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)params; // Unused parameter
    
    if (!result || result_size == 0) {
        return -1;
    }
    
    // Check if handle exists
    bool is_valid = handle_pool_is_valid(&g_handle_pool, handle_id);
    
    if (is_valid) {
        size_t memory_usage = handle_pool_get_memory_usage(&g_handle_pool, handle_id);
        snprintf(result, result_size, "{\"handle_id\":%u,\"status\":\"active\",\"memory_usage\":%zu}", 
                 handle_id, memory_usage);
    } else {
        snprintf(result, result_size, "{\"handle_id\":%u,\"status\":\"invalid\"}", handle_id);
    }
    
    return 0;
}
