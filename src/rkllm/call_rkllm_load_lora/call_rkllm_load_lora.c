#include "call_rkllm_load_lora.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_float_param/extract_float_param.h"
#include <rkllm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_load_lora(json_object* params) {
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
    
    // Get lora_adapter from object parameters
    json_object* lora_obj;
    if (!json_object_object_get_ex(params, "lora_adapter", &lora_obj)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Missing required parameter: lora_adapter"));
        return error_result;
    }
    if (!lora_obj || !json_object_is_type(lora_obj, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid lora_adapter parameter - expected object"));
        return error_result;
    }
    
    // Convert JSON to RKLLMLoraAdapter structure
    RKLLMLoraAdapter lora_adapter = {0};
    
    // Extract lora_adapter_path using jsonrpc function
    char* adapter_path = extract_string_param(lora_obj, "lora_adapter_path", NULL);
    lora_adapter.lora_adapter_path = adapter_path;
    
    // Extract lora_adapter_name using jsonrpc function
    char* adapter_name = extract_string_param(lora_obj, "lora_adapter_name", NULL);
    lora_adapter.lora_adapter_name = adapter_name;
    
    // Extract scale using jsonrpc function
    lora_adapter.scale = extract_float_param(lora_obj, "scale", 1.0f);
    
    // Call rkllm_load_lora
    int result = rkllm_load_lora(global_llm_handle, &lora_adapter);
    
    // Clean up allocated memory (extract functions return allocated strings)
    if (lora_adapter.lora_adapter_path) free(lora_adapter.lora_adapter_path);
    if (lora_adapter.lora_adapter_name) free(lora_adapter.lora_adapter_name);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("LoRA adapter loaded successfully"));
        
        return result_obj;
    } else {
        // Error occurred
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Failed to load LoRA adapter"));
        return error_result;
    }
}