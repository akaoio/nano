#include "call_rknn_inputs_set.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_inputs_set(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
    }
    
    if (!global_rknn_initialized || global_rknn_context == 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "success", json_object_new_boolean(0));
        json_object_object_add(error_result, "error", json_object_new_string("RKNN context not initialized"));
        return error_result;
    }
    
    json_object* inputs_array;
    if (!json_object_object_get_ex(params, "inputs", &inputs_array) || 
        !json_object_is_type(inputs_array, json_type_array)) {
        return NULL;
    }
    
    int n_inputs = json_object_array_length(inputs_array);
    if (n_inputs == 0) {
        return NULL;
    }
    
    rknn_input* inputs = malloc(n_inputs * sizeof(rknn_input));
    if (!inputs) {
        return NULL;
    }
    
    // Parse each input
    for (int i = 0; i < n_inputs; i++) {
        json_object* input_obj = json_object_array_get_idx(inputs_array, i);
        if (!input_obj || !json_object_is_type(input_obj, json_type_object)) {
            free(inputs);
            return NULL;
        }
        
        // Index
        json_object* index_obj;
        if (!json_object_object_get_ex(input_obj, "index", &index_obj)) {
            free(inputs);
            return NULL;
        }
        inputs[i].index = json_object_get_int(index_obj);
        
        // Buffer data (base64 encoded)
        json_object* buf_obj;
        if (!json_object_object_get_ex(input_obj, "buf", &buf_obj)) {
            free(inputs);
            return NULL;
        }
        const char* buf_str = json_object_get_string(buf_obj);
        // Note: In real implementation, decode base64 to binary data
        // For now, just use the string directly (primitive implementation)
        inputs[i].buf = (void*)buf_str;
        
        // Size
        json_object* size_obj;
        if (!json_object_object_get_ex(input_obj, "size", &size_obj)) {
            free(inputs);
            return NULL;
        }
        inputs[i].size = json_object_get_int(size_obj);
        
        // Pass through
        json_object* pass_through_obj;
        if (json_object_object_get_ex(input_obj, "pass_through", &pass_through_obj)) {
            inputs[i].pass_through = json_object_get_boolean(pass_through_obj);
        } else {
            inputs[i].pass_through = 0;
        }
        
        // Type
        json_object* type_obj;
        if (json_object_object_get_ex(input_obj, "type", &type_obj)) {
            inputs[i].type = json_object_get_int(type_obj);
        } else {
            inputs[i].type = RKNN_TENSOR_UINT8; // Default
        }
        
        // Format
        json_object* fmt_obj;
        if (json_object_object_get_ex(input_obj, "fmt", &fmt_obj)) {
            inputs[i].fmt = json_object_get_int(fmt_obj);
        } else {
            inputs[i].fmt = RKNN_TENSOR_NHWC; // Default
        }
    }
    
    // Call RKNN function
    int ret = rknn_inputs_set(global_rknn_context, n_inputs, inputs);
    
    free(inputs);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_inputs_set failed"));
    }
    
    return result;
}