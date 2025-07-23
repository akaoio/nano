#include "call_rkllm_set_chat_template.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>
#include <string.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_set_chat_template(json_object* params) {
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        return NULL; // Error: Model not initialized
    }
    
    if (!params || !json_object_is_type(params, json_type_array)) {
        return NULL; // Error: Invalid parameters
    }
    
    // Expect 4 parameters: [handle, system_prompt, prompt_prefix, prompt_postfix]
    if (json_object_array_length(params) < 4) {
        return NULL; // Error: Insufficient parameters
    }
    
    // Get system_prompt (parameter 1)
    json_object* system_obj = json_object_array_get_idx(params, 1);
    const char* system_prompt = NULL;
    if (system_obj && json_object_is_type(system_obj, json_type_string)) {
        system_prompt = json_object_get_string(system_obj);
    }
    
    // Get prompt_prefix (parameter 2)
    json_object* prefix_obj = json_object_array_get_idx(params, 2);
    const char* prompt_prefix = NULL;
    if (prefix_obj && json_object_is_type(prefix_obj, json_type_string)) {
        prompt_prefix = json_object_get_string(prefix_obj);
    }
    
    // Get prompt_postfix (parameter 3)
    json_object* postfix_obj = json_object_array_get_idx(params, 3);
    const char* prompt_postfix = NULL;
    if (postfix_obj && json_object_is_type(postfix_obj, json_type_string)) {
        prompt_postfix = json_object_get_string(postfix_obj);
    }
    
    // Call rkllm_set_chat_template
    int result = rkllm_set_chat_template(global_llm_handle, system_prompt, prompt_prefix, prompt_postfix);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("Chat template set successfully"));
        
        // Include template details in response
        if (system_prompt) {
            json_object_object_add(result_obj, "system_prompt", json_object_new_string(system_prompt));
        }
        if (prompt_prefix) {
            json_object_object_add(result_obj, "prompt_prefix", json_object_new_string(prompt_prefix));
        }
        if (prompt_postfix) {
            json_object_object_add(result_obj, "prompt_postfix", json_object_new_string(prompt_postfix));
        }
        
        return result_obj;
    } else {
        // Error occurred
        return NULL;
    }
}