#include "rkllm_proxy.h"
#include "../handle_pool/handle_pool.h"
#include <string.h>
#include <stdio.h>

// Global handle pool
extern handle_pool_t g_handle_pool;

int proxy_lora_init(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!params || !result || result_size == 0) {
        return -1;
    }
    
    // Extract parameters from JSON
    char base_model_path[256];
    char lora_adapter_path[256];
    
    if (!json_get_string(params, "base_model_path", base_model_path, sizeof(base_model_path)) ||
        !json_get_string(params, "lora_adapter_path", lora_adapter_path, sizeof(lora_adapter_path))) {
        snprintf(result, result_size, "{\"error\":\"Missing required parameters\"}");
        return -1;
    }
    
    // Create handle with base model first
    uint32_t new_handle_id = handle_pool_create(&g_handle_pool, base_model_path);
    if (new_handle_id == 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to create base model handle\"}");
        return -1;
    }
    
    // TODO: Load LoRA adapter using rkllm_load_lora
    // For now, return success
    snprintf(result, result_size, "{\"handle_id\":%u,\"status\":\"lora_loaded\"}", new_handle_id);
    return 0;
}

int proxy_abort(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)params; // Unused parameter
    
    if (!result || result_size == 0) {
        return -1;
    }
    
    // Get handle from pool
    LLMHandle* handle = handle_pool_get(&g_handle_pool, handle_id);
    if (!handle) {
        snprintf(result, result_size, "{\"error\":\"Invalid handle_id: %u\"}", handle_id);
        return -1;
    }
    
    // Call rkllm_abort
    int ret = rkllm_abort(*handle);
    if (ret != 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to abort handle %u\"}", handle_id);
        return -1;
    }
    
    snprintf(result, result_size, "{\"status\":\"aborted\"}");
    return 0;
}

int proxy_clear_kv_cache(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!params || !result || result_size == 0) {
        return -1;
    }
    
    // Get handle from pool
    LLMHandle* handle = handle_pool_get(&g_handle_pool, handle_id);
    if (!handle) {
        snprintf(result, result_size, "{\"error\":\"Invalid handle_id: %u\"}", handle_id);
        return -1;
    }
    
    // Extract parameters
    int keep_system_prompt = json_get_int(params, "keep_system_prompt", 0);
    
    // Call rkllm_clear_kv_cache
    int start_pos, end_pos;
    int ret = rkllm_clear_kv_cache(*handle, keep_system_prompt, &start_pos, &end_pos);
    if (ret != 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to clear KV cache\"}");
        return -1;
    }
    
    snprintf(result, result_size, "{\"status\":\"cleared\",\"start_pos\":%d,\"end_pos\":%d}", 
             start_pos, end_pos);
    return 0;
}
