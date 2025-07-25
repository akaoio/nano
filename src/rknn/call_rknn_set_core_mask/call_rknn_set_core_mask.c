#include "call_rknn_set_core_mask.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_set_core_mask(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
    }
    
    // Get context - can be global or specific context using jsonrpc function
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
        // Parse as pointer string
        sscanf(context_str, "%p", (void**)&context);
    }
    free(context_str);
    
    // Get core_mask parameter using jsonrpc function
    int core_mask_int = extract_int_param(params, "core_mask", 0);
    if (core_mask_int == 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("core_mask parameter is required"));
        return error_result;
    }
    rknn_core_mask core_mask = (rknn_core_mask)core_mask_int;
    
    // Call RKNN function
    int ret = rknn_set_core_mask(context, core_mask);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_set_core_mask failed"));
    }
    
    return result;
}