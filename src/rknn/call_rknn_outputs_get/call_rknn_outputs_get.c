#include "call_rknn_outputs_get.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_outputs_get(json_object* params) {
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
    
    rknn_output* outputs = malloc(n_outputs * sizeof(rknn_output));
    if (!outputs) {
        return NULL;
    }
    
    // Parse outputs array if provided for preallocation
    json_object* outputs_array;
    if (json_object_object_get_ex(params, "outputs", &outputs_array) && 
        json_object_is_type(outputs_array, json_type_array)) {
        
        int array_len = json_object_array_length(outputs_array);
        for (int i = 0; i < n_outputs && i < array_len; i++) {
            json_object* output_obj = json_object_array_get_idx(outputs_array, i);
            if (output_obj && json_object_is_type(output_obj, json_type_object)) {
                
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
                
                // Index
                json_object* index_obj;
                if (json_object_object_get_ex(output_obj, "index", &index_obj)) {
                    outputs[i].index = json_object_get_int(index_obj);
                } else {
                    outputs[i].index = i;
                }
                
                // If prealloc, need buf and size
                if (outputs[i].is_prealloc) {
                    json_object* buf_obj;
                    json_object* size_obj;
                    if (json_object_object_get_ex(output_obj, "buf", &buf_obj) &&
                        json_object_object_get_ex(output_obj, "size", &size_obj)) {
                        outputs[i].buf = (void*)json_object_get_string(buf_obj);
                        outputs[i].size = json_object_get_int(size_obj);
                    } else {
                        outputs[i].buf = NULL;
                        outputs[i].size = 0;
                    }
                } else {
                    outputs[i].buf = NULL;
                    outputs[i].size = 0;
                }
            } else {
                // Default values
                outputs[i].want_float = 0;
                outputs[i].is_prealloc = 0;
                outputs[i].index = i;
                outputs[i].buf = NULL;
                outputs[i].size = 0;
            }
        }
    } else {
        // Initialize with defaults
        for (int i = 0; i < n_outputs; i++) {
            outputs[i].want_float = 0;
            outputs[i].is_prealloc = 0;
            outputs[i].index = i;
            outputs[i].buf = NULL;
            outputs[i].size = 0;
        }
    }
    
    // Parse extend parameter if provided
    rknn_output_extend extend = {0};
    json_object* extend_obj;
    if (json_object_object_get_ex(params, "extend", &extend_obj) && 
        json_object_is_type(extend_obj, json_type_object)) {
        
        json_object* frame_id_obj;
        if (json_object_object_get_ex(extend_obj, "frame_id", &frame_id_obj)) {
            extend.frame_id = json_object_get_int64(frame_id_obj);
        }
    }
    
    // Call RKNN function
    int ret = rknn_outputs_get(global_rknn_context, n_outputs, outputs, &extend);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret == RKNN_SUCC) {
        // Create outputs array
        json_object* outputs_result = json_object_new_array();
        for (int i = 0; i < n_outputs; i++) {
            json_object* output_result = json_object_new_object();
            json_object_object_add(output_result, "index", json_object_new_int(outputs[i].index));
            json_object_object_add(output_result, "size", json_object_new_int(outputs[i].size));
            json_object_object_add(output_result, "want_float", json_object_new_boolean(outputs[i].want_float));
            json_object_object_add(output_result, "is_prealloc", json_object_new_boolean(outputs[i].is_prealloc));
            
            // For primitive implementation, return buffer pointer as string
            // In real implementation, would encode buffer data as base64
            char buf_ptr_str[32];
            snprintf(buf_ptr_str, sizeof(buf_ptr_str), "%p", outputs[i].buf);
            json_object_object_add(output_result, "buf_ptr", json_object_new_string(buf_ptr_str));
            
            json_object_array_add(outputs_result, output_result);
        }
        json_object_object_add(result, "outputs", outputs_result);
        
        // Add extend info
        json_object* extend_result = json_object_new_object();
        json_object_object_add(extend_result, "frame_id", json_object_new_int64(extend.frame_id));
        json_object_object_add(result, "extend", extend_result);
    } else {
        json_object_object_add(result, "error", json_object_new_string("rknn_outputs_get failed"));
    }
    
    free(outputs);
    return result;
}