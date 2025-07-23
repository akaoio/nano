#include "call_rkllm_load_prompt_cache.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>
#include <string.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_load_prompt_cache(json_object* params) {
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        return NULL; // Error: Model not initialized
    }
    
    if (!params || !json_object_is_type(params, json_type_array)) {
        return NULL; // Error: Invalid parameters
    }
    
    // Expect 2 parameters: [handle, prompt_cache_path]
    if (json_object_array_length(params) < 2) {
        return NULL; // Error: Insufficient parameters
    }
    
    // Get prompt_cache_path (parameter 1)
    json_object* path_obj = json_object_array_get_idx(params, 1);
    if (!path_obj || !json_object_is_type(path_obj, json_type_string)) {
        return NULL; // Error: Invalid prompt_cache_path parameter
    }
    
    const char* prompt_cache_path = json_object_get_string(path_obj);
    if (!prompt_cache_path || strlen(prompt_cache_path) == 0) {
        return NULL; // Error: Empty path
    }
    
    // Call rkllm_load_prompt_cache
    int result = rkllm_load_prompt_cache(global_llm_handle, prompt_cache_path);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("Prompt cache loaded successfully"));
        json_object_object_add(result_obj, "cache_path", json_object_new_string(prompt_cache_path));
        
        return result_obj;
    } else {
        // Error occurred
        return NULL;
    }
}