#include "call_rknn_destroy_mem.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_destroy_mem(json_object* params) {
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
    
    // Get mem parameter (pointer string)
    json_object* mem_obj;
    if (!json_object_object_get_ex(params, "mem", &mem_obj)) {
        return NULL;
    }
    const char* mem_str = json_object_get_string(mem_obj);
    rknn_tensor_mem* mem;
    sscanf(mem_str, "%p", (void**)&mem);
    
    // Call RKNN function
    int ret = rknn_destroy_mem(context, mem);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_destroy_mem failed"));
    }
    
    return result;
}