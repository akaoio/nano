#include "operations.h"
#include "handle_pool.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Global handle pool - declared as extern in io.c
extern handle_pool_t g_pool;

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

int method_init(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)handle_id;
    
    char model_path[512];
    if (!json_get_string(params, "model_path", model_path, sizeof(model_path))) {
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
    
    int ret = rkllm_init(handle, &param, NULL);
    if (ret != 0) {
        handle_pool_destroy(&g_pool, new_handle_id);
        return ret;
    }
    
    snprintf(result, result_size, "{\"handle_id\":%u}", new_handle_id);
    return 0;
}

int method_run(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!handle_pool_is_valid(&g_pool, handle_id)) return -1;
    
    LLMHandle* handle = handle_pool_get(&g_pool, handle_id);
    if (!handle) return -1;
    
    char prompt[1024];
    if (!json_get_string(params, "prompt", prompt, sizeof(prompt))) {
        return -1;
    }
    
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    RKLLMInferParam infer_param = {0};
    infer_param.mode = RKLLM_INFER_GENERATE;
    
    int ret = rkllm_run(*handle, &input, &infer_param, NULL);
    if (ret != 0) return ret;
    
    snprintf(result, result_size, "{\"text\":\"%s\"}", prompt);
    return 0;
}

int method_destroy(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    (void)params;
    
    if (!handle_pool_is_valid(&g_pool, handle_id)) return -1;
    
    LLMHandle* handle = handle_pool_get(&g_pool, handle_id);
    if (!handle) return -1;
    
    int ret = rkllm_destroy(*handle);
    if (ret != 0) return ret;
    
    handle_pool_destroy(&g_pool, handle_id);
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

static operation_t operations[] = {
    {"init", method_init},
    {"run", method_run},
    {"destroy", method_destroy},
    {"status", method_status},
    {NULL, NULL}
};

int execute_method(const char* method, uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!method || !result || result_size == 0) return -1;
    
    for (int i = 0; operations[i].method; i++) {
        if (strcmp(operations[i].method, method) == 0) {
            return operations[i].func(handle_id, params, result, result_size);
        }
    }
    return -1; // Method not found
}
