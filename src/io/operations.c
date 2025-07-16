#include "operations.h"
#include "handle_pool.h"
#include "system_info.h"
#include "model_version.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

extern handle_pool_t g_pool;

// Forward declarations
static int inference_callback(RKLLMResult* result, void* userdata, LLMCallState state);

typedef struct {
    char* output_buffer;
    size_t buffer_size;
    int finished;
} CallbackData;

// JSON parsing utilities
const char* json_get_string(const char* json, const char* key, char* buffer, size_t buffer_size) {
    if (!json || !key || !buffer || buffer_size == 0) return NULL;
    
    char key_str[256];
    snprintf(key_str, sizeof(key_str), "\"%s\":", key);
    const char* start = strstr(json, key_str);
    if (!start) return NULL;
    
    start += strlen(key_str);
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++;
    if (*start != '"') return NULL;
    start++;
    
    const char* end = strchr(start, '"');
    if (!end || (end - start) >= buffer_size) return NULL;
    
    strncpy(buffer, start, end - start);
    buffer[end - start] = '\0';
    return buffer;
}

int json_get_int(const char* json, const char* key, int default_val) {
    char buffer[64];
    const char* value = json_get_string(json, key, buffer, sizeof(buffer));
    return value ? atoi(value) : default_val;
}

double json_get_double(const char* json, const char* key, double default_val) {
    char buffer[64];
    const char* value = json_get_string(json, key, buffer, sizeof(buffer));
    return value ? atof(value) : default_val;
}

// Inference callback for streaming output
static int inference_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (!result || !userdata) return 0;
    CallbackData* data = (CallbackData*)userdata;
    
    if (state == RKLLM_RUN_NORMAL && result->text) {
        // Append to buffer
        if (data->output_buffer && data->buffer_size > 0) {
            size_t current_len = strlen(data->output_buffer);
            size_t text_len = strlen(result->text);
            if (current_len + text_len < data->buffer_size - 1) {
                strcat(data->output_buffer, result->text);
            }
        }
    } else if (state == RKLLM_RUN_FINISH) {
        data->finished = 1;
    } else if (state == RKLLM_RUN_ERROR) {
        printf("❌ Inference error occurred\n");
        data->finished = 1;
    }
    return 0;
}

int method_init(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)handle_id;
    char model_path[512];
    if (!json_get_string(params, "model_path", model_path, sizeof(model_path))) return -1;
    
    // Check model compatibility with runtime before loading
    compatibility_result_t compat_result;
    if (model_check_compatibility(model_path, &compat_result) == 0) {
        printf("🔍 Model version check: %s\n", compat_result.error_message);
        
        if (!compat_result.is_compatible) {
            snprintf(result, result_size, 
                "{\"error\":\"Model incompatible with runtime\",\"details\":\"%s\",\"model_version\":\"%s\",\"runtime_version\":\"%s\"}", 
                compat_result.error_message, 
                compat_result.model_info.version_string,
                compat_result.runtime_info.version_string);
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
            "{\"error\":\"Insufficient resources\",\"required_mb\":%lu,\"available_mb\":%lu,\"model_size_mb\":%lu}",
            model_info.memory_required_mb, sys_info.available_ram_mb, model_info.model_size_mb);
        return -1;
    }
    
    // ...existing code...
    
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
    
    snprintf(result, result_size, "{\"handle_id\":%u,\"system_info\":{\"ram_mb\":%lu,\"npu_cores\":%u,\"model_size_mb\":%lu}}", 
        new_handle_id, sys_info.total_ram_mb, sys_info.npu_cores, model_info.model_size_mb);
    return 0;
}

int method_run(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!handle_pool_is_valid(&g_pool, handle_id)) return -1;
    
    LLMHandle* handle = handle_pool_get(&g_pool, handle_id);
    if (!handle) return -1;
    
    char prompt[1024];
    if (!json_get_string(params, "prompt", prompt, sizeof(prompt))) return -1;
    
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    RKLLMInferParam infer_param = {0};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 0;
    
    char output_buffer[1024] = {0};
    CallbackData callback_data = {output_buffer, sizeof(output_buffer), 0};
    
    int ret = rkllm_run(*handle, &input, &infer_param, &callback_data);
    if (ret != 0) return ret;
    
    // Wait for completion (30s timeout)
    int timeout = 300;
    while (!callback_data.finished && timeout-- > 0) {
        usleep(100000);
    }
    
    if (!callback_data.finished) return -1;
    if (strlen(output_buffer) == 0) return -1;
    
    snprintf(result, result_size, "{\"text\":\"%s\"}", output_buffer);
    return 0;
}

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
            snprintf(result, result_size, 
                "{\"error\":\"LoRA adapter incompatible with base model\",\"details\":\"%s\",\"base_model_version\":\"%s\",\"lora_adapter_version\":\"%s\",\"runtime_version\":\"%s\"}", 
                compat_result.error_message,
                compat_result.model_info.version_string,
                compat_result.runtime_info.version_string,
                get_runtime_version_string());
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
    
    // ...existing code...
    
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
            "{\"error\":\"Insufficient resources for base+LoRA\",\"required_mb\":%lu,\"available_mb\":%lu}",
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
        printf("❌ Base model initialization failed: %d\n", ret);
        handle_pool_destroy(&g_pool, new_handle_id);
        return ret;
    }
    
    // Verify the handle was properly initialized
    if (!handle || !*handle) {
        printf("❌ Handle is null after initialization\n");
        handle_pool_destroy(&g_pool, new_handle_id);
        snprintf(result, result_size, "{\"error\":\"Handle initialization failed\"}");
        return -1;
    }
    
    // Step 2: Load LoRA adapter with better error handling
    RKLLMLoraAdapter lora_adapter = {
        .lora_adapter_path = lora_adapter_path,
        .lora_adapter_name = "default",
        .scale = 1.0f
    };
    
    printf("🔄 Loading LoRA adapter: %s\n", lora_adapter_path);
    
    // Redirect stderr to capture LoRA loading errors
    fflush(stderr);
    int stderr_backup = dup(STDERR_FILENO);
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        rkllm_destroy(*handle);
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
        printf("❌ LoRA adapter loading failed with return code: %d\n", ret);
        lora_failed = 1;
    } else if (bytes_read > 0 && (strstr(error_buffer, "AddLora: error") || 
                                  strstr(error_buffer, "failed to apply lora adapter"))) {
        printf("❌ LoRA adapter loading failed (detected in stderr): %s\n", error_buffer);
        lora_failed = 1;
        ret = -1; // Set error code
    }
    
    if (lora_failed) {
        // Careful cleanup - the base model may still be partially loaded
        printf("🧹 Cleaning up failed LoRA loading...\n");
        
        // When LoRA loading fails, the RKLLM library might be in an inconsistent state
        // Avoid calling rkllm_destroy() to prevent segfault
        printf("⚠️  Skipping rkllm_destroy() due to LoRA loading failure\n");
        printf("🔍 This prevents segfault when library is in inconsistent state\n");
        
        // Set handle to NULL to prevent further use
        if (handle) {
            *handle = NULL;
        }
        
        // Always destroy the handle pool entry
        handle_pool_destroy(&g_pool, new_handle_id);
        
        // Force memory cleanup
        system_force_gc();
        
        snprintf(result, result_size, 
            "{\"error\":\"LoRA adapter loading failed\",\"error_code\":%d,\"adapter_path\":\"%s\",\"details\":\"%s\"}", 
            ret, lora_adapter_path, bytes_read > 0 ? error_buffer : "Unknown error");
        return ret;
    }
    
    printf("✅ LoRA adapter loaded successfully\n");
    
    snprintf(result, result_size, 
        "{\"handle_id\":%u,\"system_info\":{\"ram_mb\":%lu,\"npu_cores\":%u,\"base_model_mb\":%lu,\"lora_adapter_mb\":%lu}}", 
        new_handle_id, sys_info.total_ram_mb, sys_info.npu_cores, base_model_info.model_size_mb, lora_model_info.model_size_mb);
    return 0;
}

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
