#include "call_rkllm_destroy.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_destroy(void) {
    // Check if there's a model to destroy
    if (!global_llm_initialized || !global_llm_handle) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("No model to destroy - model not initialized"));
        return error_result;
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
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32001));
        json_object_object_add(error_result, "message", json_object_new_string("Error destroying model"));
        return error_result;
    }
}