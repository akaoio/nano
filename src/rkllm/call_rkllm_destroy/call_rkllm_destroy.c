#include "call_rkllm_destroy.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_destroy(void) {
    // Check if model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        return NULL; // Error: No model to destroy
    }
    
    // Call rkllm_destroy
    int result = rkllm_destroy(global_llm_handle);
    
    if (result == 0) {
        // Success - reset global state
        global_llm_handle = NULL;
        global_llm_initialized = 0;
        
        // Return success result
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("Model destroyed successfully"));
        
        return result_obj;
    } else {
        // Error occurred
        return NULL;
    }
}