#include "call_rknn_set_batch_core_num.h"
#include <rknn_api.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_set_batch_core_num(json_object* params) {
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
    
    // Get core_num parameter
    json_object* core_num_obj;
    if (!json_object_object_get_ex(params, "core_num", &core_num_obj)) {
        return NULL;
    }
    int core_num = json_object_get_int(core_num_obj);
    
    // Call RKNN function
    int ret = rknn_set_batch_core_num(context, core_num);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_set_batch_core_num failed"));
    }
    
    return result;
}