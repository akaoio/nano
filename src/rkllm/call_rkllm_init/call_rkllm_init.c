#include "call_rkllm_init.h"
#include "../convert_json_to_rkllm_param/convert_json_to_rkllm_param.h"
#include "../convert_rkllm_result_to_json/convert_rkllm_result_to_json.h"
#include "../manage_streaming_context/manage_streaming_context.h"
#include "../../jsonrpc/format_response/format_response.h"
#include <stdbool.h>
#include <stdio.h>
#include <rkllm.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
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
    // Debug: Log callback invocation
    printf("[DEBUG] Callback called - state: %d, text: %s\n", state, result ? (result->text ? result->text : "NULL") : "result=NULL");
    
    // Get current streaming context
    StreamingContext* context = get_streaming_context();
    if (!context) {
        // No active streaming context - ignore callback (during init)
        printf("[DEBUG] No streaming context, ignoring callback\n");
        return 0;
    }
    
    // Convert RKLLM result to JSON with 1:1 mapping
    json_object* result_json = convert_rkllm_result_to_json(result, state);
    if (!result_json) {
        return 0;
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
    // Note: This function returns the result object only
    // The JSON-RPC response wrapper is handled by format_response
    if (!params || !json_object_is_type(params, json_type_array)) {
        return NULL; // Error handling done by caller
    }
    
    // Only ONE model can be loaded at a time - destroy existing if needed
    if (global_llm_initialized && global_llm_handle) {
        rkllm_destroy(global_llm_handle);
        global_llm_handle = NULL;
        global_llm_initialized = 0;
    }
    
    // Get second parameter (RKLLMParam object) - params[1] per DESIGN.md
    // params[0] = null (LLMHandle managed by server)  
    // params[1] = RKLLMParam structure
    // params[2] = null (LLMResultCallback managed by server)
    json_object* param_obj = json_object_array_get_idx(params, 1);
    if (!param_obj) {
        return NULL; // Error handling done by caller
    }
    
    // Convert JSON to RKLLMParam
    RKLLMParam rkllm_param;
    if (convert_json_to_rkllm_param(param_obj, &rkllm_param) != 0) {
        return NULL; // Error handling done by caller
    }
    // Validate model_path is provided
    if (!rkllm_param.model_path || strlen(rkllm_param.model_path) == 0) {
        return NULL; // Error handling done by caller
    }
    
    // Install signal handler for timeout
    struct sigaction old_action;
    struct sigaction timeout_action;
    timeout_action.sa_handler = timeout_handler;
    sigemptyset(&timeout_action.sa_mask);
    timeout_action.sa_flags = 0;
    sigaction(SIGALRM, &timeout_action, &old_action);
    
    // Reset timeout flag
    init_timeout = 0;
    
    // Set alarm for timeout
    alarm(30); // 30 second timeout
    
    // Call rkllm_init directly  
    int init_result = rkllm_init(&global_llm_handle, &rkllm_param, global_rkllm_callback);
    
    // Cancel alarm and restore signal handler
    alarm(0);
    sigaction(SIGALRM, &old_action, NULL);
    
    // Check if timeout occurred
    if (init_timeout) {
        // Timeout occurred during init
        return NULL;
    }
    
    // DON'T free strings yet - RKLLM library may retain pointers to them
    // They will be freed when the model is destroyed or reinitialized
    // if (rkllm_param.model_path) free((void*)rkllm_param.model_path);
    // if (rkllm_param.img_start) free((void*)rkllm_param.img_start);
    // if (rkllm_param.img_end) free((void*)rkllm_param.img_end);
    // if (rkllm_param.img_content) free((void*)rkllm_param.img_content);
    
    if (init_result != 0) {
        // RKLLM init failed - return NULL for error handling by caller
        return NULL;
    }
    
    // Success - mark as initialized
    global_llm_initialized = 1;
    
    // Return success result object only
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string("Model initialized successfully"));
    
    return result;
}