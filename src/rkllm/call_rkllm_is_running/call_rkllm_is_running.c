#include "call_rkllm_is_running.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <stdbool.h>
#include <rkllm.h>
#include <json-c/json.h>

// External reference to global LLM handle from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_is_running(void) {
    // Check if model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        return NULL; // Error: Model not initialized
    }
    
    // Call rkllm_is_running
    int is_running = rkllm_is_running(global_llm_handle);
    
    // Return result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "is_running", json_object_new_boolean(is_running == 0));
    json_object_object_add(result, "status_code", json_object_new_int(is_running));
    
    return result;
}