#include "call_rknn_set_input_shape.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../jsonrpc/extract_object_param/extract_object_param.h"
#include "../../jsonrpc/extract_array_param/extract_array_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_set_input_shape(json_object* params) {
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
    
    // Get attr parameter using jsonrpc function
    json_object* attr_obj = extract_object_param(params, "attr");
    if (!attr_obj) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("attr parameter is required"));
        return error_result;
    }
    
    rknn_tensor_attr attr;
    memset(&attr, 0, sizeof(attr));
    
    // Parse tensor attributes using jsonrpc functions
    attr.index = extract_int_param(attr_obj, "index", 0);
    attr.n_dims = extract_int_param(attr_obj, "n_dims", 0);
    
    // Handle dims array
    json_object* dims_array = extract_array_param(attr_obj, "dims");
    if (dims_array) {
        int dims_len = json_object_array_length(dims_array);
        for (int i = 0; i < dims_len && i < RKNN_MAX_DIMS; i++) {
            json_object* dim_obj = json_object_array_get_idx(dims_array, i);
            if (dim_obj && json_object_is_type(dim_obj, json_type_int)) {
                attr.dims[i] = json_object_get_int(dim_obj);
            }
        }
    }
    
    // Handle name string
    char* name_str = extract_string_param(attr_obj, "name", NULL);
    if (name_str) {
        strncpy(attr.name, name_str, RKNN_MAX_NAME_LEN - 1);
        attr.name[RKNN_MAX_NAME_LEN - 1] = '\0';
        free(name_str);
    }
    
    attr.fmt = (rknn_tensor_format)extract_int_param(attr_obj, "fmt", RKNN_TENSOR_NHWC);
    attr.type = (rknn_tensor_type)extract_int_param(attr_obj, "type", RKNN_TENSOR_FLOAT32);
    
    // Call RKNN function
    int ret = rknn_set_input_shape(context, &attr);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_set_input_shape failed"));
    }
    
    return result;
}