#include "call_rknn_set_input_shapes.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../jsonrpc/extract_array_param/extract_array_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_set_input_shapes(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters"));
        return error_result;
    }
    
    // Get context using jsonrpc function
    rknn_context context;
    char* context_str = extract_string_param(params, "context", "global");
    
    if (strcmp(context_str, "global") == 0) {
        if (!global_rknn_initialized || global_rknn_context == 0) {
            free(context_str);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32000));
            json_object_object_add(error_result, "message", json_object_new_string("Global RKNN context not initialized"));
            return error_result;
        }
        context = global_rknn_context;
    } else {
        sscanf(context_str, "%p", (void**)&context);
    }
    free(context_str);
    
    // Get n_inputs and attrs array using jsonrpc functions
    uint32_t n_inputs = (uint32_t)extract_int_param(params, "n_inputs", 0);
    if (n_inputs == 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("n_inputs parameter is required"));
        return error_result;
    }
    
    json_object* attrs_array = extract_array_param(params, "attrs");
    if (!attrs_array) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("attrs parameter is required"));
        return error_result;
    }
    
    if (json_object_array_length(attrs_array) != n_inputs) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("attrs array length must match n_inputs"));
        return error_result;
    }
    
    rknn_tensor_attr* attrs = malloc(n_inputs * sizeof(rknn_tensor_attr));
    if (!attrs) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Memory allocation failed"));
        return error_result;
    }
    
    memset(attrs, 0, n_inputs * sizeof(rknn_tensor_attr));
    
    // Parse each tensor attribute using jsonrpc functions
    for (uint32_t i = 0; i < n_inputs; i++) {
        json_object* attr_obj = json_object_array_get_idx(attrs_array, i);
        if (!attr_obj || !json_object_is_type(attr_obj, json_type_object)) {
            free(attrs);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32602));
            json_object_object_add(error_result, "message", json_object_new_string("Invalid attr object in array"));
            return error_result;
        }
        
        // Extract tensor attributes using jsonrpc functions
        attrs[i].index = extract_int_param(attr_obj, "index", i);
        attrs[i].n_dims = extract_int_param(attr_obj, "n_dims", 0);
        
        // Handle dims array
        json_object* dims_array = extract_array_param(attr_obj, "dims");
        if (dims_array) {
            int dims_len = json_object_array_length(dims_array);
            for (int j = 0; j < dims_len && j < RKNN_MAX_DIMS; j++) {
                json_object* dim_obj = json_object_array_get_idx(dims_array, j);
                if (dim_obj && json_object_is_type(dim_obj, json_type_int)) {
                    attrs[i].dims[j] = json_object_get_int(dim_obj);
                }
            }
        }
        
        // Handle name string
        char* name_str = extract_string_param(attr_obj, "name", NULL);
        if (name_str) {
            strncpy(attrs[i].name, name_str, RKNN_MAX_NAME_LEN - 1);
            attrs[i].name[RKNN_MAX_NAME_LEN - 1] = '\0';
            free(name_str);
        }
        
        attrs[i].fmt = (rknn_tensor_format)extract_int_param(attr_obj, "fmt", RKNN_TENSOR_NHWC);
        attrs[i].type = (rknn_tensor_type)extract_int_param(attr_obj, "type", RKNN_TENSOR_FLOAT32);
    }
    
    // Call RKNN function
    int ret = rknn_set_input_shapes(context, n_inputs, attrs);
    
    free(attrs);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_set_input_shapes failed"));
    }
    
    return result;
}