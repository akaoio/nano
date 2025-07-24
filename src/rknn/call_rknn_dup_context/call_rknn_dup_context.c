#include "call_rknn_dup_context.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_dup_context(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
    }
    
    json_object* context_in_obj;
    if (!json_object_object_get_ex(params, "context_in", &context_in_obj)) {
        return NULL;
    }
    
    // Parse context_in parameter - can be pointer string or use global context
    rknn_context context_in;
    if (json_object_is_type(context_in_obj, json_type_string)) {
        const char* context_in_str = json_object_get_string(context_in_obj);
        if (strcmp(context_in_str, "global") == 0) {
            if (!global_rknn_initialized || global_rknn_context == 0) {
                json_object* error_result = json_object_new_object();
                json_object_object_add(error_result, "success", json_object_new_boolean(0));
                json_object_object_add(error_result, "error", json_object_new_string("Global RKNN context not initialized"));
                return error_result;
            }
            context_in = global_rknn_context;
        } else {
            // Parse as pointer string
            sscanf(context_in_str, "%p", (void**)&context_in);
        }
    } else if (json_object_is_type(context_in_obj, json_type_int)) {
        context_in = (rknn_context)json_object_get_int64(context_in_obj);
    } else {
        return NULL;
    }
    
    rknn_context context_out = 0;
    
    // Call RKNN function
    int ret = rknn_dup_context(&context_in, &context_out);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret == RKNN_SUCC) {
        // Return context_out as pointer string
        char context_out_str[32];
        snprintf(context_out_str, sizeof(context_out_str), "%p", (void*)context_out);
        json_object_object_add(result, "context_out", json_object_new_string(context_out_str));
        json_object_object_add(result, "context_out_int", json_object_new_int64((int64_t)context_out));
    } else {
        json_object_object_add(result, "error", json_object_new_string("rknn_dup_context failed"));
    }
    
    return result;
}