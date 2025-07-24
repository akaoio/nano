#include "call_rknn_set_input_shapes.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_set_input_shapes(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
    }
    
    // Get context - can be global or specific context
    rknn_context context;
    json_object* context_obj;
    if (json_object_object_get_ex(params, "context", &context_obj)) {
        if (json_object_is_type(context_obj, json_type_string)) {
            const char* context_str = json_object_get_string(context_obj);
            if (strcmp(context_str, "global") == 0) {
                if (!global_rknn_initialized || global_rknn_context == 0) {
                    json_object* error_result = json_object_new_object();
                    json_object_object_add(error_result, "success", json_object_new_boolean(0));
                    json_object_object_add(error_result, "error", json_object_new_string("Global RKNN context not initialized"));
                    return error_result;
                }
                context = global_rknn_context;
            } else {
                sscanf(context_str, "%p", (void**)&context);
            }
        } else if (json_object_is_type(context_obj, json_type_int)) {
            context = (rknn_context)json_object_get_int64(context_obj);
        } else {
            return NULL;
        }
    } else {
        if (!global_rknn_initialized || global_rknn_context == 0) {
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "success", json_object_new_boolean(0));
            json_object_object_add(error_result, "error", json_object_new_string("Global RKNN context not initialized"));
            return error_result;
        }
        context = global_rknn_context;
    }
    
    // Get n_inputs and attr array
    json_object* n_inputs_obj;
    if (!json_object_object_get_ex(params, "n_inputs", &n_inputs_obj)) {
        return NULL;
    }
    uint32_t n_inputs = json_object_get_int(n_inputs_obj);
    
    json_object* attrs_array;
    if (!json_object_object_get_ex(params, "attrs", &attrs_array) || 
        !json_object_is_type(attrs_array, json_type_array)) {
        return NULL;
    }
    
    if (json_object_array_length(attrs_array) != n_inputs) {
        return NULL;
    }
    
    rknn_tensor_attr* attrs = malloc(n_inputs * sizeof(rknn_tensor_attr));
    if (!attrs) {
        return NULL;
    }
    
    memset(attrs, 0, n_inputs * sizeof(rknn_tensor_attr));
    
    // Parse each tensor attribute
    for (uint32_t i = 0; i < n_inputs; i++) {
        json_object* attr_obj = json_object_array_get_idx(attrs_array, i);
        if (!attr_obj || !json_object_is_type(attr_obj, json_type_object)) {
            free(attrs);
            return NULL;
        }
        
        json_object* index_obj;
        if (json_object_object_get_ex(attr_obj, "index", &index_obj)) {
            attrs[i].index = json_object_get_int(index_obj);
        }
        
        json_object* n_dims_obj;
        if (json_object_object_get_ex(attr_obj, "n_dims", &n_dims_obj)) {
            attrs[i].n_dims = json_object_get_int(n_dims_obj);
        }
        
        json_object* dims_obj;
        if (json_object_object_get_ex(attr_obj, "dims", &dims_obj) && 
            json_object_is_type(dims_obj, json_type_array)) {
            int dims_len = json_object_array_length(dims_obj);
            for (int j = 0; j < dims_len && j < RKNN_MAX_DIMS; j++) {
                json_object* dim_obj = json_object_array_get_idx(dims_obj, j);
                attrs[i].dims[j] = json_object_get_int(dim_obj);
            }
        }
        
        json_object* name_obj;
        if (json_object_object_get_ex(attr_obj, "name", &name_obj)) {
            const char* name_str = json_object_get_string(name_obj);
            strncpy(attrs[i].name, name_str, RKNN_MAX_NAME_LEN - 1);
            attrs[i].name[RKNN_MAX_NAME_LEN - 1] = '\0';
        }
        
        json_object* fmt_obj;
        if (json_object_object_get_ex(attr_obj, "fmt", &fmt_obj)) {
            attrs[i].fmt = (rknn_tensor_format)json_object_get_int(fmt_obj);
        }
        
        json_object* type_obj;
        if (json_object_object_get_ex(attr_obj, "type", &type_obj)) {
            attrs[i].type = (rknn_tensor_type)json_object_get_int(type_obj);
        }
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