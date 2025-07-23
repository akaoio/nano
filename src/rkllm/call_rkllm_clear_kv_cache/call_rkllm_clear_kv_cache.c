#include "call_rkllm_clear_kv_cache.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>
#include <stdlib.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_clear_kv_cache(json_object* params) {
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        return NULL; // Error: Model not initialized
    }
    
    if (!params || !json_object_is_type(params, json_type_array)) {
        return NULL; // Error: Invalid parameters
    }
    
    // Expect 4 parameters: [handle, keep_system_prompt, start_pos, end_pos]
    if (json_object_array_length(params) < 4) {
        return NULL; // Error: Insufficient parameters
    }
    
    // Get keep_system_prompt (parameter 1)
    json_object* keep_obj = json_object_array_get_idx(params, 1);
    int keep_system_prompt = 0;
    if (keep_obj && json_object_is_type(keep_obj, json_type_int)) {
        keep_system_prompt = json_object_get_int(keep_obj);
    }
    
    // Get start_pos array (parameter 2)
    json_object* start_pos_obj = json_object_array_get_idx(params, 2);
    int* start_pos = NULL;
    size_t start_pos_len = 0;
    
    if (start_pos_obj && json_object_is_type(start_pos_obj, json_type_array)) {
        start_pos_len = json_object_array_length(start_pos_obj);
        if (start_pos_len > 0) {
            start_pos = (int*)malloc(start_pos_len * sizeof(int));
            if (!start_pos) {
                return NULL; // Memory allocation failed
            }
            for (size_t i = 0; i < start_pos_len; i++) {
                json_object* elem = json_object_array_get_idx(start_pos_obj, i);
                if (elem && json_object_is_type(elem, json_type_int)) {
                    start_pos[i] = json_object_get_int(elem);
                }
            }
        }
    }
    
    // Get end_pos array (parameter 3)
    json_object* end_pos_obj = json_object_array_get_idx(params, 3);
    int* end_pos = NULL;
    size_t end_pos_len = 0;
    
    if (end_pos_obj && json_object_is_type(end_pos_obj, json_type_array)) {
        end_pos_len = json_object_array_length(end_pos_obj);
        if (end_pos_len > 0) {
            end_pos = (int*)malloc(end_pos_len * sizeof(int));
            if (!end_pos) {
                if (start_pos) free(start_pos);
                return NULL; // Memory allocation failed
            }
            for (size_t i = 0; i < end_pos_len; i++) {
                json_object* elem = json_object_array_get_idx(end_pos_obj, i);
                if (elem && json_object_is_type(elem, json_type_int)) {
                    end_pos[i] = json_object_get_int(elem);
                }
            }
        }
    }
    
    // Call rkllm_clear_kv_cache
    int result = rkllm_clear_kv_cache(global_llm_handle, keep_system_prompt, start_pos, end_pos);
    
    // Clean up allocated memory
    if (start_pos) free(start_pos);
    if (end_pos) free(end_pos);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("KV cache cleared successfully"));
        json_object_object_add(result_obj, "keep_system_prompt", json_object_new_int(keep_system_prompt));
        
        return result_obj;
    } else {
        // Error occurred
        return NULL;
    }
}