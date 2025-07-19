#include "rkllm_operations.h"
#include "rkllm_proxy.h"
#include "../handle_pool/handle_pool.h"
#include "../../../common/core.h"
#include "../../../libs/rkllm/rkllm.h"
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Helper function to extract string from JSON using json-c
static int json_extract_string_safe(const char* json_str, const char* key, char* buffer, size_t buffer_size) {
    if (!json_str || !key || !buffer || buffer_size == 0) {
        return -1;
    }
    
    json_object *root = json_tokener_parse(json_str);
    if (!root) {
        return -1;
    }
    
    json_object *value;
    if (!json_object_object_get_ex(root, key, &value)) {
        json_object_put(root);
        return -1;
    }
    
    const char *str_value = json_object_get_string(value);
    if (!str_value) {
        json_object_put(root);
        return -1;
    }
    
    strncpy(buffer, str_value, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    json_object_put(root);
    return 0;
}

// Callback data structure for collecting RKLLM output
typedef struct {
    char* output_buffer;
    size_t buffer_size;
    size_t current_pos;
    bool finished;
    int error_code;
} callback_data_t;

// RKLLM callback function - collects actual output
int rkllm_output_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    callback_data_t* data = (callback_data_t*)userdata;
    
    if (!data || !result) {
        return 0;
    }
    
    // Collect generated text
    if (result->text && strlen(result->text) > 0) {
        size_t text_len = strlen(result->text);
        if (data->current_pos + text_len < data->buffer_size - 1) {
            strcat(data->output_buffer + data->current_pos, result->text);
            data->current_pos += text_len;
        }
    }
    
    // Check state
    switch (state) {
        case RKLLM_RUN_FINISH:
            data->finished = true;
            break;
        case RKLLM_RUN_ERROR:
            data->error_code = -1;
            data->finished = true;
            break;
        case RKLLM_RUN_NORMAL:
        case RKLLM_RUN_WAITING:
            // Continue processing
            break;
    }
    
    return 0; // Continue inference
}

// Helper functions
extern LLMHandle rkllm_proxy_get_handle(uint32_t handle_id);
extern char* rkllm_proxy_create_json_result(int status, const char* data);
extern char* rkllm_proxy_create_error_result(int status, const char* error_msg);

// Operation handlers implementation

int rkllm_op_init(uint32_t* handle_id, const char* params_json, rkllm_result_t* result) {
    if (!handle_id || !params_json || !result) {
        return -1;
    }
    
    // Parse JSON parameters
    // Expected format: {"model_path": "path/to/model.rkllm", "param": {...}}
    char model_path[512] = {0};
    
    if (json_extract_string_safe(params_json, "model_path", model_path, sizeof(model_path)) != 0) {
        SET_ERROR_RESULT(result, -1, "model_path required");
        return -1;
    }
    
    // Create handle in pool
    uint32_t new_handle_id = handle_pool_create(&g_handle_pool, model_path);
    if (new_handle_id == 0) {
        SET_ERROR_RESULT(result, -1, "Failed to create handle");
        return -1;
    }
    
    // Now actually initialize the RKLLM handle
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = model_path;
    param.max_new_tokens = 100;
    param.temperature = 0.7f;
    param.top_p = 0.9f;
    
    LLMHandle llm_handle = nullptr;
    int rkllm_ret = rkllm_init(&llm_handle, &param, rkllm_proxy_global_callback);
    
    if (rkllm_ret != 0 || llm_handle == nullptr) {
        (void)handle_pool_destroy(&g_handle_pool, new_handle_id);
        SET_ERROR_RESULT(result, -1, "Failed to initialize RKLLM handle");
        return -1;
    }
    
    // Set the actual handle in the pool
    (void)handle_pool_set_handle(&g_handle_pool, new_handle_id, llm_handle);
    
    *handle_id = new_handle_id;
    
    // Create success result using json-c
    json_object *result_obj = json_object_new_object();
    json_object *handle_id_obj = json_object_new_int(new_handle_id);
    json_object_object_add(result_obj, "handle_id", handle_id_obj);
    
    const char *result_str = json_object_to_json_string(result_obj);
    result->result_data = rkllm_proxy_create_json_result(0, result_str);
    result->result_size = strlen(result->result_data);
    
    json_object_put(result_obj);
    return 0;
}

int rkllm_op_destroy(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    (void)params_json; // Unused
    
    if (!result) {
        return -1;
    }
    
    // Get handle before destroying from pool
    LLMHandle handle = rkllm_proxy_get_handle(handle_id);
    if (handle) {
        // Properly destroy RKLLM handle to free NPU memory
        rkllm_destroy(handle);
    }
    
    // Remove from handle pool
    int status = handle_pool_destroy(&g_handle_pool, handle_id);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, nullptr);
    } else {
        result->result_data = rkllm_proxy_create_error_result(-1, "Failed to destroy handle");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_run(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = get_validated_handle_or_error(handle_id, result);
    if (!handle) return -1;
    
    // Parse JSON parameters for RKLLMInput and RKLLMInferParam
    // Extract prompt from JSON  
    char prompt[1024] = {0};
    
    // Parse prompt - required parameter
    if (json_extract_string_safe(params_json, "prompt", prompt, sizeof(prompt)) != 0 || strlen(prompt) == 0) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Missing required parameter: prompt");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Create input structure
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    RKLLMInferParam infer_param = {0};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 1;
    
    // Create callback context to capture output
    rkllm_callback_context_t* callback_context = rkllm_proxy_create_callback_context(2048);
    if (!callback_context) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Failed to create callback context");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    int status = rkllm_run(handle, &input, &infer_param, callback_context);
    
    if (status == 0 && callback_context->output_buffer && callback_context->current_pos > 0) {
        // Use json-c to format output
        const char *output_str = callback_context->output_buffer;
        result->result_data = rkllm_proxy_create_json_result(0, output_str);
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Run failed or no output");
    }
    
    result->result_size = strlen(result->result_data);
    
    // Clean up callback context
    rkllm_proxy_destroy_callback_context(callback_context);
    
    return status;
}

// Forward declaration for streaming callback from IO worker
extern void io_streaming_chunk_callback(const char* chunk, bool is_final, void* userdata);

int rkllm_op_run_streaming(uint32_t handle_id, const char* params_json, rkllm_result_t* result, uint32_t request_id) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = get_validated_handle_or_error(handle_id, result);
    if (!handle) return -1;
    
    // Parse JSON parameters for RKLLMInput and RKLLMInferParam
    // Extract prompt from JSON  
    char prompt[1024] = {0};
    
    // Parse prompt - required parameter
    if (json_extract_string_safe(params_json, "prompt", prompt, sizeof(prompt)) != 0 || strlen(prompt) == 0) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Missing required parameter: prompt");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Create input structure
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    RKLLMInferParam infer_param = {0};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 1;
    
    // Create callback context for streaming
    rkllm_callback_context_t* callback_context = rkllm_proxy_create_callback_context(2048);
    if (!callback_context) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Failed to create callback context");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Enable streaming mode
    callback_context->streaming_enabled = true;
    callback_context->request_id = request_id;
    callback_context->stream_callback = io_streaming_chunk_callback;
    callback_context->stream_userdata = &callback_context->request_id;
    
    int status = rkllm_run(handle, &input, &infer_param, callback_context);
    
    // For streaming, return success immediately after starting
    // The actual response will be streamed via callback
    result->result_data = rkllm_proxy_create_json_result(0, "Streaming started");
    result->result_size = strlen(result->result_data);
    
    // Clean up callback context
    rkllm_proxy_destroy_callback_context(callback_context);
    
    return status;
}

int rkllm_op_run_async_streaming(uint32_t handle_id, const char* params_json, rkllm_result_t* result, uint32_t request_id) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = get_validated_handle_or_error(handle_id, result);
    if (!handle) return -1;
    
    // Parse JSON parameters for RKLLMInput and RKLLMInferParam
    // Extract prompt from JSON  
    char prompt[1024] = {0};
    
    // Parse prompt - required parameter
    if (json_extract_string_safe(params_json, "prompt", prompt, sizeof(prompt)) != 0 || strlen(prompt) == 0) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Missing required parameter: prompt");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Create input structure
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    RKLLMInferParam infer_param = {0};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 1;
    
    // Create callback context for streaming
    rkllm_callback_context_t* callback_context = rkllm_proxy_create_callback_context(2048);
    if (!callback_context) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Failed to create callback context");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Enable streaming mode
    callback_context->streaming_enabled = true;
    callback_context->request_id = request_id;
    callback_context->stream_callback = io_streaming_chunk_callback;
    callback_context->stream_userdata = &callback_context->request_id;
    
    int status = rkllm_run_async(handle, &input, &infer_param, callback_context);
    
    // For streaming, return success immediately after starting
    // The actual response will be streamed via callback
    result->result_data = rkllm_proxy_create_json_result(0, "Async streaming started");
    result->result_size = strlen(result->result_data);
    
    // Clean up callback context
    rkllm_proxy_destroy_callback_context(callback_context);
    
    return status;
}

int rkllm_op_run_async(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = rkllm_proxy_get_handle(handle_id);
    if (!handle) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Invalid handle");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Parse JSON parameters using json-c
    char prompt[1024] = {0};
    
    // Parse prompt using json-c
    json_object *root = json_tokener_parse(params_json);
    if (root) {
        json_object *prompt_obj;
        if (json_object_object_get_ex(root, "prompt", &prompt_obj)) {
            const char *prompt_str = json_object_get_string(prompt_obj);
            if (prompt_str) {
                strncpy(prompt, prompt_str, sizeof(prompt) - 1);
                prompt[sizeof(prompt) - 1] = '\0';
            }
        }
        json_object_put(root);
    }
    
    // Create input structure
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    RKLLMInferParam infer_param = {0};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 1;
    
    // Create callback context to capture output
    rkllm_callback_context_t* callback_context = rkllm_proxy_create_callback_context(2048);
    if (!callback_context) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Failed to create callback context");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    int status = rkllm_run_async(handle, &input, &infer_param, callback_context);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, "\"Async processing started\"");
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Async run failed");
    }
    
    result->result_size = strlen(result->result_data);
    
    // Don't destroy context for async - it will be used by the callback
    // TODO: Need proper async context management
    
    return status;
}

int rkllm_op_abort(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    (void)params_json; // Unused
    
    if (!result) {
        return -1;
    }
    
    LLMHandle handle = get_validated_handle_or_error(handle_id, result);
    if (!handle) return -1;
    
    int status = rkllm_abort(handle);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, nullptr);
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Abort failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_is_running(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    (void)params_json; // Unused
    
    if (!result) {
        return -1;
    }
    
    LLMHandle handle = get_validated_handle_or_error(handle_id, result);
    if (!handle) return -1;
    
    int is_running = rkllm_is_running(handle);
    
    // Use json-c to create boolean result
    json_object *result_obj = json_object_new_boolean(is_running);
    const char *result_str = json_object_to_json_string(result_obj);
    result->result_data = rkllm_proxy_create_json_result(0, result_str);
    result->result_size = strlen(result->result_data);
    
    json_object_put(result_obj);
    return 0;
}

int rkllm_op_load_lora(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = get_validated_handle_or_error(handle_id, result);
    if (!handle) return -1;
    
    // Parse lora adapter parameters from JSON
    char lora_path[256] = {0};
    
    // Parse lora adapter path - required parameter
    if (json_extract_string_safe(params_json, "path", lora_path, sizeof(lora_path)) != 0 || strlen(lora_path) == 0) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Missing required parameter: path");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    RKLLMLoraAdapter adapter = {0};
    adapter.lora_adapter_path = lora_path;
    adapter.lora_adapter_name = nullptr;
    adapter.scale = 1.0f;
    
    int status = rkllm_load_lora(handle, &adapter);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, "\"Lora loaded\"");
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Load lora failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

// Implement remaining operations with similar pattern
int rkllm_op_load_prompt_cache(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = get_validated_handle_or_error(handle_id, result);
    if (!handle) return -1;
    
    // Parse prompt cache path from JSON
    char cache_path[256] = {0};
    
    // Parse cache path - required parameter
    if (json_extract_string_safe(params_json, "path", cache_path, sizeof(cache_path)) != 0 || strlen(cache_path) == 0) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Missing required parameter: path");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    int status = rkllm_load_prompt_cache(handle, cache_path);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, "\"Prompt cache loaded\"");
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Load prompt cache failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_release_prompt_cache(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    (void)params_json; // Unused
    
    if (!result) {
        return -1;
    }
    
    LLMHandle handle = get_validated_handle_or_error(handle_id, result);
    if (!handle) return -1;
    
    int status = rkllm_release_prompt_cache(handle);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, "\"Prompt cache released\"");
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Release prompt cache failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_clear_kv_cache(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    (void)params_json; // Unused
    
    if (!result) {
        return -1;
    }
    
    LLMHandle handle = rkllm_proxy_get_handle(handle_id);
    if (!handle) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Invalid handle");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    int start_pos = 0, end_pos = 0;
    int status = rkllm_clear_kv_cache(handle, 1, &start_pos, &end_pos);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, "\"KV cache cleared\"");
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Clear KV cache failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_get_kv_cache_size(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    (void)params_json; // Unused
    
    if (!result) {
        return -1;
    }
    
    LLMHandle handle = rkllm_proxy_get_handle(handle_id);
    if (!handle) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Invalid handle");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    int cache_sizes[2] = {0, 0};
    int status = rkllm_get_kv_cache_size(handle, cache_sizes);
    
    if (status == 0) {
        // Use json-c to create integer result
        json_object *result_obj = json_object_new_int(cache_sizes[0]);
        const char *result_str = json_object_to_json_string(result_obj);
        result->result_data = rkllm_proxy_create_json_result(0, result_str);
        json_object_put(result_obj);
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Get KV cache size failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_set_chat_template(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = rkllm_proxy_get_handle(handle_id);
    if (!handle) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Invalid handle");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Parse template from JSON using json-c
    char system_prompt[512] = "";
    char prompt_prefix[128] = "";
    char prompt_postfix[128] = "";
    
    json_object *root = json_tokener_parse(params_json);
    if (root) {
        json_object *system_obj;
        if (json_object_object_get_ex(root, "system_prompt", &system_obj)) {
            const char *system_str = json_object_get_string(system_obj);
            if (system_str) {
                strncpy(system_prompt, system_str, sizeof(system_prompt) - 1);
                system_prompt[sizeof(system_prompt) - 1] = '\0';
            }
        }
        
        json_object *prefix_obj;
        if (json_object_object_get_ex(root, "prompt_prefix", &prefix_obj)) {
            const char *prefix_str = json_object_get_string(prefix_obj);
            if (prefix_str) {
                strncpy(prompt_prefix, prefix_str, sizeof(prompt_prefix) - 1);
                prompt_prefix[sizeof(prompt_prefix) - 1] = '\0';
            }
        }
        
        json_object *postfix_obj;
        if (json_object_object_get_ex(root, "prompt_postfix", &postfix_obj)) {
            const char *postfix_str = json_object_get_string(postfix_obj);
            if (postfix_str) {
                strncpy(prompt_postfix, postfix_str, sizeof(prompt_postfix) - 1);
                prompt_postfix[sizeof(prompt_postfix) - 1] = '\0';
            }
        }
        
        json_object_put(root);
    }
    
    int status = rkllm_set_chat_template(handle, system_prompt, prompt_prefix, prompt_postfix);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, "\"Chat template set\"");
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Set chat template failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_set_function_tools(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = rkllm_proxy_get_handle(handle_id);
    if (!handle) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Invalid handle");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Parse function tools from JSON with proper parameters
    char system_prompt[512] = "";
    char tools_str[2048] = "";
    char tool_response_str[64] = "";
    
    const char* system_start = strstr(params_json, "\"system_prompt\":");
    if (system_start) {
        system_start = strchr(system_start + 15, '"');
        if (system_start) {
            system_start++;
            const char* system_end = strchr(system_start, '"');
            if (system_end) {
                size_t len = system_end - system_start;
                if (len < sizeof(system_prompt)) {
                    strncpy(system_prompt, system_start, len);
                    system_prompt[len] = '\0';
                }
            }
        }
    }
    
    const char* tools_start = strstr(params_json, "\"tools\":");
    if (tools_start) {
        tools_start = strchr(tools_start + 7, '"');
        if (tools_start) {
            tools_start++;
            const char* tools_end = strchr(tools_start, '"');
            if (tools_end) {
                size_t len = tools_end - tools_start;
                if (len < sizeof(tools_str)) {
                    strncpy(tools_str, tools_start, len);
                    tools_str[len] = '\0';
                }
            }
        }
    }
    
    const char* response_start = strstr(params_json, "\"tool_response_str\":");
    if (response_start) {
        response_start = strchr(response_start + 19, '"');
        if (response_start) {
            response_start++;
            const char* response_end = strchr(response_start, '"');
            if (response_end) {
                size_t len = response_end - response_start;
                if (len < sizeof(tool_response_str)) {
                    strncpy(tool_response_str, response_start, len);
                    tool_response_str[len] = '\0';
                }
            }
        }
    }
    
    int status = rkllm_set_function_tools(handle, system_prompt, tools_str, tool_response_str);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, "\"Function tools set\"");
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Set function tools failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_set_cross_attn_params(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    if (!params_json || !result) {
        return -1;
    }
    
    LLMHandle handle = rkllm_proxy_get_handle(handle_id);
    if (!handle) {
        result->result_data = rkllm_proxy_create_error_result(-1, "Invalid handle");
        result->result_size = strlen(result->result_data);
        return -1;
    }
    
    // Parse cross attention parameters from JSON
    RKLLMCrossAttnParam cross_attn_params = {0};
    
    // Parse num_tokens
    const char* num_tokens_start = strstr(params_json, "\"num_tokens\":");
    if (num_tokens_start) {
        cross_attn_params.num_tokens = (int)strtol(num_tokens_start + 13, nullptr, 10);
    }
    
    // For actual implementation, encoder caches and masks would need to be provided
    // This is a basic structure initialization
    
    int status = rkllm_set_cross_attn_params(handle, &cross_attn_params);
    
    if (status == 0) {
        result->result_data = rkllm_proxy_create_json_result(0, "\"Cross attention params set\"");
    } else {
        result->result_data = rkllm_proxy_create_error_result(status, "Set cross attention params failed");
    }
    
    result->result_size = strlen(result->result_data);
    return status;
}

int rkllm_op_create_default_param(uint32_t handle_id, const char* params_json, rkllm_result_t* result) {
    (void)handle_id; (void)params_json;
    
    if (!result) {
        return -1;
    }
    
    RKLLMParam param = rkllm_createDefaultParam();
    
    // Convert param to JSON using json-c
    json_object *result_obj = json_object_new_object();
    json_object *model_path_obj = json_object_new_string(param.model_path ? param.model_path : "");
    json_object *max_context_obj = json_object_new_int(param.max_context_len);
    json_object *max_tokens_obj = json_object_new_int(param.max_new_tokens);
    json_object *temp_obj = json_object_new_double(param.temperature);
    json_object *top_p_obj = json_object_new_double(param.top_p);
    
    json_object_object_add(result_obj, "model_path", model_path_obj);
    json_object_object_add(result_obj, "max_context_len", max_context_obj);
    json_object_object_add(result_obj, "max_new_tokens", max_tokens_obj);
    json_object_object_add(result_obj, "temperature", temp_obj);
    json_object_object_add(result_obj, "top_p", top_p_obj);
    
    const char *result_str = json_object_to_json_string(result_obj);
    
    result->result_data = rkllm_proxy_create_json_result(0, result_str);
    result->result_size = strlen(result->result_data);
    
    json_object_put(result_obj);
    
    return 0;
}

// Operation handler table
const rkllm_op_handler_t OPERATION_HANDLERS[OP_MAX] = {
    [OP_INIT] = nullptr, // Special case handled separately
    [OP_DESTROY] = rkllm_op_destroy,
    [OP_RUN] = rkllm_op_run,
    [OP_RUN_ASYNC] = rkllm_op_run_async,
    [OP_ABORT] = rkllm_op_abort,
    [OP_IS_RUNNING] = rkllm_op_is_running,
    [OP_LOAD_LORA] = rkllm_op_load_lora,
    [OP_LOAD_PROMPT_CACHE] = rkllm_op_load_prompt_cache,
    [OP_RELEASE_PROMPT_CACHE] = rkllm_op_release_prompt_cache,
    [OP_CLEAR_KV_CACHE] = rkllm_op_clear_kv_cache,
    [OP_GET_KV_CACHE_SIZE] = rkllm_op_get_kv_cache_size,
    [OP_SET_CHAT_TEMPLATE] = rkllm_op_set_chat_template,
    [OP_SET_FUNCTION_TOOLS] = rkllm_op_set_function_tools,
    [OP_SET_CROSS_ATTN_PARAMS] = rkllm_op_set_cross_attn_params,
    [OP_CREATE_DEFAULT_PARAM] = rkllm_op_create_default_param
};

// Init handler
const rkllm_op_init_handler_t INIT_HANDLER = rkllm_op_init;
