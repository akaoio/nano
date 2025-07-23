#include "call_rkllm_run.h"
#include "../convert_json_to_rkllm_input/convert_json_to_rkllm_input.h"
#include "../convert_json_to_rkllm_infer_param/convert_json_to_rkllm_infer_param.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include "../manage_streaming_context/manage_streaming_context.h"
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
        return NULL; // Error: Model not initialized
    }
    
    if (!params || !json_object_is_type(params, json_type_array)) {
        return NULL; // Error: Invalid parameters
    }
    
    // Expect 4 parameters: [handle, rkllm_input, rkllm_infer_params, userdata]
    if (json_object_array_length(params) < 4) {
        return NULL; // Error: Insufficient parameters
    }
    
    // Get RKLLMInput (parameter 1)
    json_object* input_obj = json_object_array_get_idx(params, 1);
    if (!input_obj) {
        return NULL;
    }
    
    RKLLMInput rkllm_input;
    if (convert_json_to_rkllm_input(input_obj, &rkllm_input) != 0) {
        return NULL; // Error: Failed to convert input
    }
    
    // Get RKLLMInferParam (parameter 2)
    json_object* infer_obj = json_object_array_get_idx(params, 2);
    if (!infer_obj) {
        return NULL;
    }
    
    RKLLMInferParam rkllm_infer_param;
    if (convert_json_to_rkllm_infer_param(infer_obj, &rkllm_infer_param) != 0) {
        return NULL; // Error: Failed to convert infer param
    }
    
    // CRITICAL FIX: Check if model was initialized with is_async=true
    // If async mode, use streaming; if sync mode, return single response
    
    // Set streaming context for the callback to capture streaming data
    set_streaming_context(client_fd, request_id);
    printf("[DEBUG] Set streaming context for rkllm_run (async mode)\n");
    
    // Call rkllm_run - the callback will handle ALL responses including final
    int result = rkllm_run(global_llm_handle, &rkllm_input, &rkllm_infer_param, NULL);
    
    printf("[DEBUG] rkllm_run returned: %d\n", result);
    
    // Clean up allocated memory for input structures
    if (rkllm_input.role) free((void*)rkllm_input.role);
    
    switch (rkllm_input.input_type) {
        case RKLLM_INPUT_PROMPT:
            if (rkllm_input.prompt_input) free((void*)rkllm_input.prompt_input);
            break;
        case RKLLM_INPUT_TOKEN:
            if (rkllm_input.token_input.input_ids) free(rkllm_input.token_input.input_ids);
            break;
        case RKLLM_INPUT_EMBED:
            if (rkllm_input.embed_input.embed) free(rkllm_input.embed_input.embed);
            break;
        case RKLLM_INPUT_MULTIMODAL:
            if (rkllm_input.multimodal_input.prompt) free(rkllm_input.multimodal_input.prompt);
            if (rkllm_input.multimodal_input.image_embed) free(rkllm_input.multimodal_input.image_embed);
            break;
    }
    
    // Clean up infer param structures
    if (rkllm_infer_param.lora_params) {
        if (rkllm_infer_param.lora_params->lora_adapter_name) {
            free((void*)rkllm_infer_param.lora_params->lora_adapter_name);
        }
        free(rkllm_infer_param.lora_params);
    }
    
    if (rkllm_infer_param.prompt_cache_params) {
        if (rkllm_infer_param.prompt_cache_params->prompt_cache_path) {
            free((void*)rkllm_infer_param.prompt_cache_params->prompt_cache_path);
        }
        free(rkllm_infer_param.prompt_cache_params);
    }
    
    if (result != 0) {
        // RKLLM run failed - clear streaming context and return error
        clear_streaming_context();
        return NULL;
    }
    
    // CRITICAL FIX: For async mode, return NULL to indicate "no immediate response"
    // The callback function will handle ALL responses to the client
    // Do NOT clear streaming context here - callback will clear it when done
    printf("[DEBUG] Async mode: returning NULL (callback handles responses)\n");
    return NULL; // No immediate response - callback handles everything
}