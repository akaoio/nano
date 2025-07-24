#include "call_rkllm_run.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include "../manage_streaming_context/manage_streaming_context.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../utils/log_message/log_message.h"
#include <stdbool.h>
#include <stdio.h>
#include <rkllm.h>
#include <string.h>
#include <stdlib.h>

// External reference to global LLM handle from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_run(json_object* params, int client_fd, int request_id) {
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Model not initialized - call rkllm.init first"));
        return error_result;
    }
    
    if (!params || !json_object_is_type(params, json_type_array)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters - expected array"));
        return error_result;
    }
    
    // Expect 4 parameters: [handle, rkllm_input, rkllm_infer_params, userdata]
    if (json_object_array_length(params) < 4) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Insufficient parameters - expected [handle, rkllm_input, rkllm_infer_params, userdata]"));
        return error_result;
    }
    
    // Get RKLLMInput (parameter 1) - convert using individual extraction functions
    json_object* input_obj = json_object_array_get_idx(params, 1);
    if (!input_obj) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid input parameter"));
        return error_result;
    }
    
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
    
    // CRITICAL FIX: Handle different inference modes with proper validation
    LOG_INFO_MSG("Inference mode: %d", rkllm_infer_param.mode);
    
    // Mode-specific validation and warnings
    switch (rkllm_infer_param.mode) {
        case 0: // RKLLM_INFER_GENERATE
            LOG_INFO_MSG("Running text generation mode");
            break;
        case 1: // RKLLM_INFER_GET_LAST_HIDDEN_LAYER
            LOG_INFO_MSG("Running hidden states extraction mode");
            break;
        case 2: // RKLLM_INFER_GET_LOGITS
            LOG_INFO_MSG("Running logits extraction mode - checking RKLLM compatibility");
            break;
        default:
            LOG_WARN_MSG("Unknown inference mode: %d", rkllm_infer_param.mode);
            break;
    }
    
    // Set streaming context for the callback to capture streaming data
    set_streaming_context(client_fd, request_id);
    LOG_DEBUG_MSG("Set streaming context for rkllm_run (mode: %d)", rkllm_infer_param.mode);
    
    // Call rkllm_run - the callback will handle ALL responses including final
    LOG_INFO_MSG("Calling rkllm_run...");
    int result = rkllm_run(global_llm_handle, &rkllm_input, &rkllm_infer_param, NULL);
    
    LOG_INFO_MSG("rkllm_run returned: %d", result);
    
    if (result != 0) {
        // RKLLM run failed - clear streaming context and return error
        LOG_ERROR_MSG("rkllm_run failed with code: %d", result);
        clear_streaming_context();
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("RKLLM run failed"));
        return error_result;
    }
    
    // CRITICAL FIX: For async mode, return NULL to indicate "no immediate response"
    // The callback function will handle ALL responses to the client
    LOG_DEBUG_MSG("Async mode: returning NULL (callback handles responses)");
    return NULL; // No immediate response - callback handles everything
}