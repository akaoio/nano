#include "call_rknn_set_input_shape.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_set_input_shape(json_object* params) {
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
                // Parse as pointer string
                sscanf(context_str, "%p", (void**)&context);
            }
        } else if (json_object_is_type(context_obj, json_type_int)) {
            context = (rknn_context)json_object_get_int64(context_obj);
        } else {
            return NULL;
        }
    } else {
        // Default to global context
        if (!global_rknn_initialized || global_rknn_context == 0) {
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "success", json_object_new_boolean(0));
            json_object_object_add(error_result, "error", json_object_new_string("Global RKNN context not initialized"));
            return error_result;
        }
        context = global_rknn_context;
    }
    
    // Get attr parameter
    json_object* attr_obj;
    if (!json_object_object_get_ex(params, "attr", &attr_obj) || 
        !json_object_is_type(attr_obj, json_type_object)) {
        return NULL;
    }
    
    rknn_tensor_attr attr;
    memset(&attr, 0, sizeof(attr));
    
    // Parse tensor attributes
    json_object* index_obj;
    if (json_object_object_get_ex(attr_obj, "index", &index_obj)) {
        attr.index = json_object_get_int(index_obj);
    }
    
    json_object* n_dims_obj;
    if (json_object_object_get_ex(attr_obj, "n_dims", &n_dims_obj)) {
        attr.n_dims = json_object_get_int(n_dims_obj);
    }
    
    json_object* dims_obj;
    if (json_object_object_get_ex(attr_obj, "dims", &dims_obj) && 
        json_object_is_type(dims_obj, json_type_array)) {
        int dims_len = json_object_array_length(dims_obj);
        for (int i = 0; i < dims_len && i < RKNN_MAX_DIMS; i++) {
            json_object* dim_obj = json_object_array_get_idx(dims_obj, i);
            attr.dims[i] = json_object_get_int(dim_obj);
        }
    }
    
    json_object* name_obj;
    if (json_object_object_get_ex(attr_obj, "name", &name_obj)) {
        const char* name_str = json_object_get_string(name_obj);
        strncpy(attr.name, name_str, RKNN_MAX_NAME_LEN - 1);
        attr.name[RKNN_MAX_NAME_LEN - 1] = '\0';
    }
    
    json_object* fmt_obj;
    if (json_object_object_get_ex(attr_obj, "fmt", &fmt_obj)) {
        attr.fmt = (rknn_tensor_format)json_object_get_int(fmt_obj);
    }
    
    json_object* type_obj;
    if (json_object_object_get_ex(attr_obj, "type", &type_obj)) {
        attr.type = (rknn_tensor_type)json_object_get_int(type_obj);
    }
    
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