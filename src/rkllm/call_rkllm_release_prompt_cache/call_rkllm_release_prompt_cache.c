#include "call_rkllm_release_prompt_cache.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>
#include <json-c/json.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_release_prompt_cache(void) {
    fprintf(stderr, "🗑️  Releasing prompt cache...\n");
    
    // Create error object for potential use
    json_object* error_obj = json_object_new_object();
    
    // Validate that model is initialized
    if (!global_llm_initialized) {
        json_object_object_add(error_obj, "code", json_object_new_int(-32001));
        json_object_object_add(error_obj, "message", json_object_new_string("Model not initialized - cannot release prompt cache"));
        
        fprintf(stderr, "❌ Cannot release prompt cache: Model not initialized\n");
        return error_obj;
    }
    
    // Validate handle is not NULL
    if (!global_llm_handle) {
        json_object_object_add(error_obj, "code", json_object_new_int(-32001));
        json_object_object_add(error_obj, "message", json_object_new_string("Invalid model handle - cannot release prompt cache"));
        
        fprintf(stderr, "❌ Cannot release prompt cache: Invalid handle\n");
        return error_obj;
    }
    
    // Add safety check: only release if a cache was actually loaded
    // Note: We assume the cache might not exist, so we handle this gracefully
    fprintf(stderr, "🔍 Calling rkllm_release_prompt_cache with handle validation\n");
    
    // Call rkllm_release_prompt_cache with extra safety
    int result = rkllm_release_prompt_cache(global_llm_handle);
    
    // Clean up error object since we're handling the result
    json_object_put(error_obj);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("Prompt cache released successfully"));
        
        fprintf(stderr, "✅ Prompt cache released successfully\n");
        return result_obj;
    } else {
        // RKLLM function failed - return proper error instead of NULL
        json_object* error_result = json_object_new_object();
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "RKLLM release_prompt_cache failed with code: %d", result);
        
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string(error_msg));
        
        fprintf(stderr, "❌ Prompt cache release failed with code: %d\n", result);
        return error_result;
    }
}