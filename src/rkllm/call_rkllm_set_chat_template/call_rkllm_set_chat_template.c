#include "call_rkllm_set_chat_template.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include <rkllm.h>
#include <stdio.h>
#include <string.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_set_chat_template(json_object* params) {
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
    
    // Get template configuration from object parameters
    json_object* config_obj;
    if (!json_object_object_get_ex(params, "template_config", &config_obj)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Missing required parameter: template_config"));
        return error_result;
    }
    if (!config_obj || !json_object_is_type(config_obj, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid template configuration - expected object"));
        return error_result;
    }
    
    // Extract template parameters using individual extraction functions
    char* system_prompt = extract_string_param(config_obj, "system_prompt", NULL);
    char* prompt_prefix = extract_string_param(config_obj, "prompt_prefix", NULL);  
    char* prompt_postfix = extract_string_param(config_obj, "prompt_postfix", NULL);
    
    // Handle chat_template field if provided (alternative format)
    if (!system_prompt && !prompt_prefix && !prompt_postfix) {
        char* chat_template = extract_string_param(config_obj, "chat_template", NULL);
        if (chat_template) {
            // Use chat_template as system_prompt for now
            system_prompt = chat_template;
        } else {
            // Cleanup and return error
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32602));
            json_object_object_add(error_result, "message", json_object_new_string("No template configuration provided"));
            return error_result;
        }
    }
    
    // Call rkllm_set_chat_template
    int result = rkllm_set_chat_template(global_llm_handle, system_prompt, prompt_prefix, prompt_postfix);
    
    // Clean up allocated strings
    if (system_prompt) free(system_prompt);
    if (prompt_prefix) free(prompt_prefix);
    if (prompt_postfix) free(prompt_postfix);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("Chat template set successfully"));
        return result_obj;
    } else {
        // Error occurred
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Failed to set chat template"));
        return error_result;
    }
}