#include "call_rknn_outputs_release.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_outputs_release(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
    }
    
    if (!global_rknn_initialized || global_rknn_context == 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "success", json_object_new_boolean(0));
        json_object_object_add(error_result, "error", json_object_new_string("RKNN context not initialized"));
        return error_result;
    }
    
    json_object* n_outputs_obj;
    if (!json_object_object_get_ex(params, "n_outputs", &n_outputs_obj)) {
        return NULL;
    }
    int n_outputs = json_object_get_int(n_outputs_obj);
    
    if (n_outputs <= 0) {
        return NULL;
    }
    
    json_object* outputs_array;
    if (!json_object_object_get_ex(params, "outputs", &outputs_array) || 
        !json_object_is_type(outputs_array, json_type_array)) {
        return NULL;
    }
    
    int array_len = json_object_array_length(outputs_array);
    if (array_len != n_outputs) {
        return NULL;
    }
    
    rknn_output* outputs = malloc(n_outputs * sizeof(rknn_output));
    if (!outputs) {
        return NULL;
    }
    
    // Parse each output
    for (int i = 0; i < n_outputs; i++) {
        json_object* output_obj = json_object_array_get_idx(outputs_array, i);
        if (!output_obj || !json_object_is_type(output_obj, json_type_object)) {
            free(outputs);
            return NULL;
        }
        
        // Index
        json_object* index_obj;
        if (json_object_object_get_ex(output_obj, "index", &index_obj)) {
            outputs[i].index = json_object_get_int(index_obj);
        } else {
            outputs[i].index = i;
        }
        
        // Want float
        json_object* want_float_obj;
        if (json_object_object_get_ex(output_obj, "want_float", &want_float_obj)) {
            outputs[i].want_float = json_object_get_boolean(want_float_obj);
        } else {
            outputs[i].want_float = 0;
        }
        
        // Is prealloc
        json_object* is_prealloc_obj;
        if (json_object_object_get_ex(output_obj, "is_prealloc", &is_prealloc_obj)) {
            outputs[i].is_prealloc = json_object_get_boolean(is_prealloc_obj);
        } else {
            outputs[i].is_prealloc = 0;
        }
        
        // Buffer pointer (from previous rknn_outputs_get call)
        json_object* buf_ptr_obj;
        if (json_object_object_get_ex(output_obj, "buf_ptr", &buf_ptr_obj)) {
            const char* buf_ptr_str = json_object_get_string(buf_ptr_obj);
            // Convert string pointer back to void*
            void* buf_ptr;
            sscanf(buf_ptr_str, "%p", &buf_ptr);
            outputs[i].buf = buf_ptr;
        } else {
            outputs[i].buf = NULL;
        }
        
        // Size
        json_object* size_obj;
        if (json_object_object_get_ex(output_obj, "size", &size_obj)) {
            outputs[i].size = json_object_get_int(size_obj);
        } else {
            outputs[i].size = 0;
        }
    }
    
    // Call RKNN function
    int ret = rknn_outputs_release(global_rknn_context, n_outputs, outputs);
    
    free(outputs);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_outputs_release failed"));
    }
    
    return result;
}