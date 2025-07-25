#include "call_rkllm_abort.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include "../manage_streaming_context/manage_streaming_context.h"
#include "../../jsonrpc/format_response/format_response.h"
#include <stdbool.h>
#include <rkllm.h>
#include <json-c/json.h>

// External reference to global LLM handle from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_abort(void) {
    // Check if model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Model not initialized - call rkllm.init first"));
        return error_result;
    }
    
    // Call rkllm_abort
    int result = rkllm_abort(global_llm_handle);
    
    // Clear any active streaming context since task was aborted
    clear_streaming_context();
    
    // Return result
    json_object* result_obj = json_object_new_object();
    json_object_object_add(result_obj, "success", json_object_new_boolean(result == 0));
    json_object_object_add(result_obj, "message", json_object_new_string(result == 0 ? "Task aborted successfully" : "Failed to abort task"));
    json_object_object_add(result_obj, "status_code", json_object_new_int(result));
    
    return result_obj;
}