#include "call_rkllm_set_cross_attn_params.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include <rkllm.h>
#include <stdio.h>
#include <stdlib.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_set_cross_attn_params(json_object* params) {
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        return NULL; // Error: Model not initialized
    }
    
    if (!params || !json_object_is_type(params, json_type_array)) {
        return NULL; // Error: Invalid parameters
    }
    
    // Expect 2 parameters: [handle, cross_attn_params]
    if (json_object_array_length(params) < 2) {
        return NULL; // Error: Insufficient parameters
    }
    
    // Get cross_attn_params (parameter 1)
    json_object* cross_attn_obj = json_object_array_get_idx(params, 1);
    if (!cross_attn_obj || !json_object_is_type(cross_attn_obj, json_type_object)) {
        return NULL; // Error: Invalid cross_attn_params parameter
    }
    
    // Convert JSON to RKLLMCrossAttnParam structure
    RKLLMCrossAttnParam cross_attn_params = {0};
    
    // Get num_tokens (required field)
    json_object* num_tokens_obj;
    if (!json_object_object_get_ex(cross_attn_obj, "num_tokens", &num_tokens_obj) ||
        !json_object_is_type(num_tokens_obj, json_type_int)) {
        return NULL; // Error: num_tokens is required
    }
    cross_attn_params.num_tokens = json_object_get_int(num_tokens_obj);
    
    if (cross_attn_params.num_tokens <= 0) {
        return NULL; // Error: Invalid num_tokens
    }
    
    // Extract encoder_k_cache array (optional)
    json_object* k_cache_obj;
    if (json_object_object_get_ex(cross_attn_obj, "encoder_k_cache", &k_cache_obj) &&
        json_object_is_type(k_cache_obj, json_type_array)) {
        size_t k_cache_len = json_object_array_length(k_cache_obj);
        if (k_cache_len > 0) {
            cross_attn_params.encoder_k_cache = (float*)malloc(k_cache_len * sizeof(float));
            for (size_t i = 0; i < k_cache_len; i++) {
                json_object* elem = json_object_array_get_idx(k_cache_obj, i);
                if (elem && json_object_is_type(elem, json_type_double)) {
                    cross_attn_params.encoder_k_cache[i] = (float)json_object_get_double(elem);
                }
            }
        }
    }
    
    // Extract encoder_v_cache array (optional)
    json_object* v_cache_obj;
    if (json_object_object_get_ex(cross_attn_obj, "encoder_v_cache", &v_cache_obj) &&
        json_object_is_type(v_cache_obj, json_type_array)) {
        size_t v_cache_len = json_object_array_length(v_cache_obj);
        if (v_cache_len > 0) {
            cross_attn_params.encoder_v_cache = (float*)malloc(v_cache_len * sizeof(float));
            for (size_t i = 0; i < v_cache_len; i++) {
                json_object* elem = json_object_array_get_idx(v_cache_obj, i);
                if (elem && json_object_is_type(elem, json_type_double)) {
                    cross_attn_params.encoder_v_cache[i] = (float)json_object_get_double(elem);
                }
            }
        }
    }
    
    // Extract encoder_mask array (optional)
    json_object* mask_obj;
    if (json_object_object_get_ex(cross_attn_obj, "encoder_mask", &mask_obj) &&
        json_object_is_type(mask_obj, json_type_array)) {
        size_t mask_len = json_object_array_length(mask_obj);
        if (mask_len > 0) {
            cross_attn_params.encoder_mask = (float*)malloc(mask_len * sizeof(float));
            for (size_t i = 0; i < mask_len; i++) {
                json_object* elem = json_object_array_get_idx(mask_obj, i);
                if (elem && json_object_is_type(elem, json_type_double)) {
                    cross_attn_params.encoder_mask[i] = (float)json_object_get_double(elem);
                }
            }
        }
    }
    
    // Extract encoder_pos array (optional)
    json_object* pos_obj;
    if (json_object_object_get_ex(cross_attn_obj, "encoder_pos", &pos_obj) &&
        json_object_is_type(pos_obj, json_type_array)) {
        size_t pos_len = json_object_array_length(pos_obj);
        if (pos_len > 0) {
            cross_attn_params.encoder_pos = (int32_t*)malloc(pos_len * sizeof(int32_t));
            for (size_t i = 0; i < pos_len; i++) {
                json_object* elem = json_object_array_get_idx(pos_obj, i);
                if (elem && json_object_is_type(elem, json_type_int)) {
                    cross_attn_params.encoder_pos[i] = json_object_get_int(elem);
                }
            }
        }
    }
    
    // Call rkllm_set_cross_attn_params
    int result = rkllm_set_cross_attn_params(global_llm_handle, &cross_attn_params);
    
    // Clean up allocated memory
    if (cross_attn_params.encoder_k_cache) free(cross_attn_params.encoder_k_cache);
    if (cross_attn_params.encoder_v_cache) free(cross_attn_params.encoder_v_cache);
    if (cross_attn_params.encoder_mask) free(cross_attn_params.encoder_mask);
    if (cross_attn_params.encoder_pos) free(cross_attn_params.encoder_pos);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("Cross-attention parameters set successfully"));
        json_object_object_add(result_obj, "num_tokens", json_object_new_int(cross_attn_params.num_tokens));
        
        return result_obj;
    } else {
        // Error occurred
        return NULL;
    }
}