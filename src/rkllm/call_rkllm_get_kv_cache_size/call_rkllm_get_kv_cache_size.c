#include "call_rkllm_get_kv_cache_size.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>
#include <stdlib.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_get_kv_cache_size(void) {
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        return NULL; // Error: Model not initialized
    }
    
    // Allocate array for cache sizes (assuming max 8 batches for safety)
    int cache_sizes[8] = {0};
    
    // Call rkllm_get_kv_cache_size
    int result = rkllm_get_kv_cache_size(global_llm_handle, cache_sizes);
    
    if (result == 0) {
        // Success - create JSON array with cache sizes
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("KV cache size retrieved successfully"));
        
        // Create array of cache sizes
        json_object* sizes_array = json_object_new_array();
        for (int i = 0; i < 8; i++) {
            if (cache_sizes[i] > 0) {
                json_object_array_add(sizes_array, json_object_new_int(cache_sizes[i]));
            }
        }
        json_object_object_add(result_obj, "cache_sizes", sizes_array);
        
        return result_obj;
    } else {
        // Error occurred
        return NULL;
    }
}