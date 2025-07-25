#include "call_rkllm_run_async.h"
#include "../manage_streaming_context/manage_streaming_context.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../utils/log_message/log_message.h"
#include <stdbool.h>
#include <stdio.h>
#include <rkllm.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// External reference to global LLM handle and callback from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;
extern int global_rkllm_callback(RKLLMResult* result, void* userdata, LLMCallState state);

json_object* call_rkllm_run_async(json_object* params, int client_fd, int request_id) {
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Model not initialized - call rkllm.init first"));
        return error_result;
    }
    
    if (!params || !json_object_is_type(params, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters - expected object"));
        return error_result;
    }
    
    // Use params directly as the input object - standardized format
    json_object* input_obj = params;
    
    RKLLMInput rkllm_input;
    memset(&rkllm_input, 0, sizeof(RKLLMInput));
    
    // Extract input_type
    rkllm_input.input_type = extract_int_param(input_obj, "input_type", RKLLM_INPUT_PROMPT);
    
    // Extract role
    char* role = extract_string_param(input_obj, "role", "user");
    if (role) {
        rkllm_input.role = role;  // Don't free - RKLLM keeps reference
    }
    
    // Handle union based on input_type
    switch (rkllm_input.input_type) {
        case RKLLM_INPUT_PROMPT: {
            char* prompt = extract_string_param(input_obj, "prompt_input", NULL);
            if (prompt) {
                rkllm_input.prompt_input = prompt;  // Don't free - RKLLM keeps reference
            }
            break;
        }
        // Add other input types as needed
    }
    
    // Get RKLLMInferParam (parameter 2) - convert using individual extraction functions
    json_object* infer_obj = json_object_array_get_idx(params, 2);
    if (!infer_obj) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid inference parameter"));
        return error_result;
    }
    
    RKLLMInferParam rkllm_infer_param;
    memset(&rkllm_infer_param, 0, sizeof(RKLLMInferParam));
    
    // Extract mode
    rkllm_infer_param.mode = extract_int_param(infer_obj, "mode", 0);
    rkllm_infer_param.keep_history = extract_int_param(infer_obj, "keep_history", 0);
    
    // Set streaming context for callback forwarding
    set_streaming_context(client_fd, request_id);
    LOG_DEBUG_MSG("About to call rkllm_run_async...");
    
    // Call rkllm_run_async with global callback
    int result = rkllm_run_async(global_llm_handle, &rkllm_input, &rkllm_infer_param, NULL);
    
    if (result != 0) {
        // RKLLM run_async failed - clear streaming context and return error
        clear_streaming_context();
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Failed to start async inference"));
        return error_result;
    }
    
    // Return success result using individual JSON functions
    json_object* result_obj = json_object_new_object();
    json_object_object_add(result_obj, "success", json_object_new_boolean(1));
    json_object_object_add(result_obj, "message", json_object_new_string("Async inference started"));
    
    return result_obj;
}

