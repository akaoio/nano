#include "call_rkllm_set_function_tools.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>
#include <string.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_set_function_tools(json_object* params) {
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
    
    // Get system_prompt from object parameters
    json_object* system_obj;
    const char* system_prompt = NULL;
    if (json_object_object_get_ex(params, "system_prompt", &system_obj) && json_object_is_type(system_obj, json_type_string)) {
        system_prompt = json_object_get_string(system_obj);
    }
    
    // Get tools from object parameters - JSON-formatted string
    json_object* tools_obj;
    const char* tools = NULL;
    if (json_object_object_get_ex(params, "tools", &tools_obj) && json_object_is_type(tools_obj, json_type_string)) {
        tools = json_object_get_string(tools_obj);
    }
    
    // Get tool_response_str from object parameters
    json_object* response_obj;
    const char* tool_response_str = NULL;
    if (json_object_object_get_ex(params, "tool_response_str", &response_obj) && json_object_is_type(response_obj, json_type_string)) {
        tool_response_str = json_object_get_string(response_obj);
    }
    
    // Call rkllm_set_function_tools
    int result = rkllm_set_function_tools(global_llm_handle, system_prompt, tools, tool_response_str);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("Function tools configuration set successfully"));
        
        // Include configuration details in response
        if (system_prompt) {
            json_object_object_add(result_obj, "system_prompt", json_object_new_string(system_prompt));
        }
        if (tools) {
            json_object_object_add(result_obj, "tools", json_object_new_string(tools));
        }
        if (tool_response_str) {
            json_object_object_add(result_obj, "tool_response_str", json_object_new_string(tool_response_str));
        }
        
        return result_obj;
    } else {
        // Error occurred
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32001));
        json_object_object_add(error_result, "message", json_object_new_string("Failed to set function tools configuration"));
        return error_result;
    }
}