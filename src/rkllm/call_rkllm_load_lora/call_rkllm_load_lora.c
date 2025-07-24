#include "call_rkllm_load_lora.h"
#include "../call_rkllm_init/call_rkllm_init.h"
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
    
    if (!params || !json_object_is_type(params, json_type_array)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters - expected array"));
        return error_result;
    }
    
    // Expect 2 parameters: [handle, lora_adapter]
    if (json_object_array_length(params) < 2) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Insufficient parameters - expected [handle, lora_adapter]"));
        return error_result;
    }
    
    // Get lora_adapter (parameter 1)
    json_object* lora_obj = json_object_array_get_idx(params, 1);
    if (!lora_obj || !json_object_is_type(lora_obj, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid lora_adapter parameter - expected object"));
        return error_result;
    }
    
    // Convert JSON to RKLLMLoraAdapter structure
    RKLLMLoraAdapter lora_adapter = {0};
    
    // Extract lora_adapter_path
    json_object* path_obj;
    if (json_object_object_get_ex(lora_obj, "lora_adapter_path", &path_obj)) {
        if (json_object_is_type(path_obj, json_type_string)) {
            const char* path_str = json_object_get_string(path_obj);
            lora_adapter.lora_adapter_path = strdup(path_str);
            if (!lora_adapter.lora_adapter_path) {
                json_object* error_result = json_object_new_object();
                json_object_object_add(error_result, "code", json_object_new_int(-32000));
                json_object_object_add(error_result, "message", json_object_new_string("Memory allocation failed"));
                return error_result;
            }
        }
    }
    
    // Extract lora_adapter_name
    json_object* name_obj;
    if (json_object_object_get_ex(lora_obj, "lora_adapter_name", &name_obj)) {
        if (json_object_is_type(name_obj, json_type_string)) {
            const char* name_str = json_object_get_string(name_obj);
            lora_adapter.lora_adapter_name = strdup(name_str);
            if (!lora_adapter.lora_adapter_name) {
                if (lora_adapter.lora_adapter_path) free(lora_adapter.lora_adapter_path);
                json_object* error_result = json_object_new_object();
                json_object_object_add(error_result, "code", json_object_new_int(-32000));
                json_object_object_add(error_result, "message", json_object_new_string("Memory allocation failed"));
                return error_result;
            }
        }
    }
    
    // Extract scale
    json_object* scale_obj;
    if (json_object_object_get_ex(lora_obj, "scale", &scale_obj)) {
        if (json_object_is_type(scale_obj, json_type_double)) {
            lora_adapter.scale = (float)json_object_get_double(scale_obj);
        }
    } else {
        lora_adapter.scale = 1.0f; // Default scale
    }
    
    // Call rkllm_load_lora
    int result = rkllm_load_lora(global_llm_handle, &lora_adapter);
    
    // Clean up allocated memory
    if (lora_adapter.lora_adapter_path) free((void*)lora_adapter.lora_adapter_path);
    if (lora_adapter.lora_adapter_name) free((void*)lora_adapter.lora_adapter_name);
    
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