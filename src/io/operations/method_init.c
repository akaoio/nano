#include "operations.h"
#include "../handle_pool.h"
#include "../system_info.h"
#include "../model_version.h"
#include <stdio.h>
#include <string.h>

extern handle_pool_t g_pool;

int method_init(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)handle_id;
    char model_path[512];
    if (!json_get_string(params, "model_path", model_path, sizeof(model_path))) return -1;
    
    // Check model compatibility with runtime before loading
    compatibility_result_t compat_result;
    if (model_check_compatibility(model_path, &compat_result) == 0) {
        printf("🔍 Model version check: %s\n", compat_result.error_message);
        
        if (!compat_result.is_compatible) {
            snprintf(result, result_size, "{\"error\":\"%s\"}", compat_result.error_message);
            return -1;
        }
    } else {
        printf("⚠️  Could not check model compatibility, proceeding anyway\n");
    }
    
    // System resource detection
    system_info_t sys_info;
    if (system_detect(&sys_info) != 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to detect system resources\"}");
        return -1;
    }
    
    // Model analysis
    model_info_t model_info;
    if (model_analyze(model_path, &sys_info, &model_info) != 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to analyze model file\"}");
        return -1;
    }
    
    // Resource availability check
    if (!system_can_load_model(&sys_info, &model_info)) {
        snprintf(result, result_size, 
            "{\"error\":\"Insufficient resources: need %luMB, have %luMB available (model: %luMB)\"}",
            model_info.memory_required_mb, sys_info.available_ram_mb, model_info.model_size_mb);
        return -1;
    }
    
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = model_path;
    param.max_context_len = json_get_int(params, "max_context_len", param.max_context_len);
    param.temperature = json_get_double(params, "temperature", param.temperature);
    param.top_p = json_get_double(params, "top_p", param.top_p);
    
    uint32_t new_handle_id = handle_pool_create(&g_pool, model_path);
    if (new_handle_id == 0) return -1;
    
    LLMHandle* handle = handle_pool_get(&g_pool, new_handle_id);
    if (!handle) return -1;
    
    int ret = rkllm_init(handle, &param, inference_callback);
    if (ret != 0) {
        handle_pool_destroy(&g_pool, new_handle_id);
        return ret;
    }
    
    snprintf(result, result_size, 
        "{\"handle_id\":%u,\"system_info\":{\"ram_mb\":%lu,\"npu_cores\":%u,\"model_size_mb\":%lu}}", 
        new_handle_id, sys_info.total_ram_mb, sys_info.npu_cores, model_info.model_size_mb);
    return 0;
}
