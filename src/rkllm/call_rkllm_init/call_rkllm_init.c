#define _POSIX_C_SOURCE 200809L

#include "call_rkllm_init.h"
#include "../manage_streaming_context/manage_streaming_context.h"
#include "../../jsonrpc/format_response/format_response.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../jsonrpc/extract_float_param/extract_float_param.h"
#include "../../jsonrpc/extract_object_param/extract_object_param.h"
#include "../../utils/log_message/log_message.h"
#include "../../utils/global_config/global_config.h"
#include <stdbool.h>
#include <stdio.h>
#include <rkllm.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Global state - only ONE model can be loaded at a time
LLMHandle global_llm_handle = NULL;
int global_llm_initialized = 0;

// Signal handler for timeout
static volatile int init_timeout = 0;
void timeout_handler(int sig) {
    init_timeout = 1;
}

int global_rkllm_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    // Log callback invocation for monitoring
    LOG_DEBUG_MSG("Callback called - state: %d, text: %s", state, result ? (result->text ? result->text : "NULL") : "result=NULL");
    
    // Get current streaming context
    StreamingContext* context = get_streaming_context();
    if (!context) {
        // No active streaming context - ignore callback (during init)
        LOG_DEBUG_MSG("No streaming context, ignoring callback");
        return 0;
    }
    
    // Convert RKLLM result to JSON using individual functions
    json_object* result_json = json_object_new_object();
    if (result) {
        // Add text field
        if (result->text) {
            json_object_object_add(result_json, "text", json_object_new_string(result->text));
        } else {
            json_object_object_add(result_json, "text", NULL);
        }
        
        // Add token_id field
        json_object_object_add(result_json, "token_id", json_object_new_int(result->token_id));
        
        // Add callback state
        json_object_object_add(result_json, "_callback_state", json_object_new_int(state));
    }
    
    // Create JSON-RPC response with streaming data
    json_object* id_obj = json_object_new_int(context->request_id);
    char* response_str = format_response(id_obj, result_json);
    json_object_put(id_obj);
    json_object_put(result_json);
    
    if (!response_str) {
        return 0;
    }
    
    // Send streaming response directly to client
    ssize_t bytes_sent = send(context->client_fd, response_str, strlen(response_str), MSG_NOSIGNAL);
    bytes_sent += send(context->client_fd, "\n", 1, MSG_NOSIGNAL);
    free(response_str);
    
    // Clear context if this is the final state
    if (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR) {
        clear_streaming_context();
    }
    
    (void)userdata;
    // Always return 0 to continue inference normally 
    // According to rkllm.h: 0=continue, 1=pause (we never want to pause)
    return 0;
}

// Structure for passing data to init thread
typedef struct {
    RKLLMParam param;
    int* result;
    pthread_mutex_t* mutex;
    pthread_cond_t* cond;
    int* finished;
} InitThreadData;

// Thread function for rkllm_init
void* rkllm_init_thread(void* arg) {
    InitThreadData* data = (InitThreadData*)arg;
    
    // Call rkllm_init in separate thread context
    int init_result = rkllm_init(&global_llm_handle, &data->param, global_rkllm_callback);
    
    // Signal completion
    pthread_mutex_lock(data->mutex);
    *(data->result) = init_result;
    *(data->finished) = 1;
    pthread_cond_signal(data->cond);
    pthread_mutex_unlock(data->mutex);
    
    return NULL;
}

json_object* call_rkllm_init(json_object* params) {
    // Standardized: Accept JSON object parameters directly
    if (!params || !json_object_is_type(params, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters - expected object"));
        return error_result;
    }
    
    // Only ONE model can be loaded at a time - destroy existing if needed
    if (global_llm_initialized && global_llm_handle) {
        rkllm_destroy(global_llm_handle);
        global_llm_handle = NULL;
        global_llm_initialized = 0;
    }
    
    // Extract model_path - REQUIRED field (directly from params)
    char* model_path = extract_string_param(params, "model_path", NULL);
    if (!model_path) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("model_path is required"));
        return error_result;
    }
    
    // Convert JSON to RKLLMParam using individual extraction functions
    RKLLMParam rkllm_param = rkllm_createDefaultParam();
    
    // Get nested param object for RKLLM parameters
    json_object* param_obj = extract_object_param(params, "param");
    if (param_obj) {
        // Extract parameters from nested param object
        rkllm_param.max_context_len = extract_int_param(param_obj, "max_context_len", rkllm_param.max_context_len);
        rkllm_param.max_new_tokens = extract_int_param(param_obj, "max_new_tokens", rkllm_param.max_new_tokens);
        rkllm_param.top_k = extract_int_param(param_obj, "top_k", rkllm_param.top_k);
        rkllm_param.top_p = extract_float_param(param_obj, "top_p", rkllm_param.top_p);
        rkllm_param.temperature = extract_float_param(param_obj, "temperature", rkllm_param.temperature);
        rkllm_param.repeat_penalty = extract_float_param(param_obj, "repeat_penalty", rkllm_param.repeat_penalty);
        // num_npu_core is handled through model initialization, not RKLLMParam
    }
    
    // Set model path - CRITICAL!
    rkllm_param.model_path = model_path;
    
    // Add critical multimodal parameters like the working example
    rkllm_param.skip_special_token = true;
    rkllm_param.img_start = "<|vision_start|>";
    rkllm_param.img_end = "<|vision_end|>";
    rkllm_param.img_content = "<|image_pad|>";
    rkllm_param.extend_param.base_domain_id = 1;
    
    // Debug logging
    LOG_INFO_MSG("RKLLM Init Debug - model_path: %s", model_path);
    LOG_INFO_MSG("RKLLM Init Debug - max_context_len: %d", rkllm_param.max_context_len);
    LOG_INFO_MSG("RKLLM Init Debug - max_new_tokens: %d", rkllm_param.max_new_tokens);
    LOG_INFO_MSG("RKLLM Init Debug - temperature: %f", rkllm_param.temperature);
    LOG_INFO_MSG("RKLLM Init Debug - top_k: %d", rkllm_param.top_k);
    
    // Install signal handler for timeout
    struct sigaction old_action;
    struct sigaction timeout_action;
    timeout_action.sa_handler = timeout_handler;
    sigemptyset(&timeout_action.sa_mask);
    timeout_action.sa_flags = 0;
    sigaction(SIGALRM, &timeout_action, &old_action);
    
    // Reset timeout flag
    init_timeout = 0;
    
    // Set alarm for configurable timeout
    int timeout_seconds = get_init_timeout() / 1000; // Convert ms to seconds
    if (timeout_seconds <= 0) timeout_seconds = 30; // Fallback to 30 seconds
    alarm(timeout_seconds);
    
    // Call rkllm_init directly  
    int init_result = rkllm_init(&global_llm_handle, &rkllm_param, global_rkllm_callback);
    
    // Cancel alarm and restore signal handler
    alarm(0);
    sigaction(SIGALRM, &old_action, NULL);
    
    // Check if timeout occurred
    if (init_timeout) {
        // Timeout occurred during init
        if (model_path) free(model_path);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Model initialization timeout"));
        return error_result;
    }
    
    // Clean up allocated strings
    if (model_path) free(model_path);
    
    if (init_result != 0) {
        // RKLLM init failed - return proper error instead of NULL
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("RKLLM initialization failed"));
        return error_result;
    }
    
    // Success - mark as initialized
    global_llm_initialized = 1;
    
    // Return success result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string("Model initialized successfully"));
    
    return result;
}