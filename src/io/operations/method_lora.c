#include "operations.h"
#include "../handle_pool.h"
#include "../system_info.h"
#include "../model_version.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

extern handle_pool_t g_pool;

int method_lora_init(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)handle_id;
    
    char base_model_path[512], lora_adapter_path[512];
    if (!json_get_string(params, "base_model_path", base_model_path, sizeof(base_model_path)) ||
        !json_get_string(params, "lora_adapter_path", lora_adapter_path, sizeof(lora_adapter_path))) {
        return -1;
    }
    
    // Check LoRA compatibility BEFORE loading
    compatibility_result_t compat_result;
    if (model_check_lora_compatibility(base_model_path, lora_adapter_path, &compat_result) == 0) {
        printf("🔍 LoRA compatibility check: %s\n", compat_result.error_message);
        
        if (!compat_result.is_compatible) {
            snprintf(result, result_size, "{\"error\":\"%s\"}", compat_result.error_message);
            return -1;
        }
    } else {
        printf("⚠️  Could not check LoRA compatibility, proceeding anyway\n");
    }
    
    // System resource detection
    system_info_t sys_info;
    if (system_detect(&sys_info) != 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to detect system resources\"}");
        return -1;
    }
    
    // Analyze base model
    model_info_t base_model_info;
    if (model_analyze(base_model_path, &sys_info, &base_model_info) != 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to analyze base model file\"}");
        return -1;
    }
    
    // Analyze LoRA adapter
    model_info_t lora_model_info;
    if (model_analyze(lora_adapter_path, &sys_info, &lora_model_info) != 0) {
        snprintf(result, result_size, "{\"error\":\"Failed to analyze LoRA adapter file\"}");
        return -1;
    }
    
    // Check combined resource requirements
    uint64_t total_memory_needed = base_model_info.memory_required_mb + lora_model_info.memory_required_mb;
    if (total_memory_needed > sys_info.available_ram_mb) {
        snprintf(result, result_size, 
            "{\"error\":\"Insufficient resources: need %luMB, have %luMB available\"}", 
            total_memory_needed, sys_info.available_ram_mb);
        return -1;
    }
    
    // Step 1: Initialize base model
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = base_model_path;
    param.max_context_len = json_get_int(params, "max_context_len", param.max_context_len);
    param.temperature = json_get_double(params, "temperature", param.temperature);
    param.top_p = json_get_double(params, "top_p", param.top_p);
    
    uint32_t new_handle_id = handle_pool_create(&g_pool, base_model_path);
    if (new_handle_id == 0) return -1;
    
    LLMHandle* handle = handle_pool_get(&g_pool, new_handle_id);
    if (!handle) {
        handle_pool_destroy(&g_pool, new_handle_id);
        snprintf(result, result_size, "{\"error\":\"Failed to get handle from pool\"}");
        return -1;
    }
    
    int ret = rkllm_init(handle, &param, inference_callback);
    if (ret != 0) {
        handle_pool_destroy(&g_pool, new_handle_id);
        snprintf(result, result_size, "{\"error\":\"Failed to initialize base model\"}");
        return ret;
    }
    
    // Verify the handle was properly initialized
    if (!handle || !*handle) {
        handle_pool_destroy(&g_pool, new_handle_id);
        snprintf(result, result_size, "{\"error\":\"Handle not properly initialized\"}");
        return -1;
    }
    
    // Step 2: Load LoRA adapter with better error handling
    RKLLMLoraAdapter lora_adapter = {0};
    lora_adapter.lora_adapter_path = lora_adapter_path;
    
    printf("🔄 Loading LoRA adapter: %s\n", lora_adapter_path);
    
    // Redirect stderr to capture LoRA loading errors
    fflush(stderr);
    int stderr_backup = dup(STDERR_FILENO);
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        handle_pool_destroy(&g_pool, new_handle_id);
        snprintf(result, result_size, "{\"error\":\"Failed to create error capture pipe\"}");
        return -1;
    }
    
    // Set non-blocking mode on read end
    int flags = fcntl(pipe_fd[0], F_GETFL);
    fcntl(pipe_fd[0], F_SETFL, flags | O_NONBLOCK);
    
    dup2(pipe_fd[1], STDERR_FILENO);
    close(pipe_fd[1]);
    
    ret = rkllm_load_lora(*handle, &lora_adapter);
    
    // Restore stderr immediately
    fflush(stderr);
    dup2(stderr_backup, STDERR_FILENO);
    close(stderr_backup);
    
    // Check for error messages in captured stderr
    char error_buffer[1024] = {0};
    int bytes_read = read(pipe_fd[0], error_buffer, sizeof(error_buffer) - 1);
    close(pipe_fd[0]);
    
    // Null-terminate the error buffer
    if (bytes_read > 0 && bytes_read < sizeof(error_buffer)) {
        error_buffer[bytes_read] = '\0';
    }
    
    // Check both return code and stderr output for errors
    int lora_failed = 0;
    if (ret != 0) {
        lora_failed = 1;
    } else if (bytes_read > 0 && (strstr(error_buffer, "AddLora: error") || 
                                  strstr(error_buffer, "failed to apply lora adapter"))) {
        lora_failed = 1;
    }
    
    if (lora_failed) {
        printf("❌ LoRA adapter loading failed (detected in stderr): %s\n", error_buffer);
        printf("🧹 Cleaning up failed LoRA loading...\n");
        
        // Don't call rkllm_destroy here to avoid segfault
        // Just clean up the handle pool entry
        handle_pool_destroy(&g_pool, new_handle_id);
        
        snprintf(result, result_size, "{\"error\":\"LoRA adapter loading failed: %s\"}", error_buffer);
        return -1;
    }
    
    printf("✅ LoRA adapter loaded successfully\n");
    
    snprintf(result, result_size, 
        "{\"handle_id\":%u,\"system_info\":{\"ram_mb\":%lu,\"npu_cores\":%u,\"base_model_size_mb\":%lu,\"lora_size_mb\":%lu}}", 
        new_handle_id, sys_info.total_ram_mb, sys_info.npu_cores, base_model_info.model_size_mb, lora_model_info.model_size_mb);
    return 0;
}
