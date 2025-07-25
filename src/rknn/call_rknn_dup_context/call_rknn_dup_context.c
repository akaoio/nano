#include "call_rknn_dup_context.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_dup_context(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters"));
        return error_result;
    }
    
    // Get context_in using jsonrpc function
    char* context_in_str = extract_string_param(params, "context_in", NULL);
    if (!context_in_str) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("context_in parameter is required"));
        return error_result;
    }
    
    // Parse context_in parameter - can be pointer string or use global context
    rknn_context context_in;
    if (strcmp(context_in_str, "global") == 0) {
        if (!global_rknn_initialized || global_rknn_context == 0) {
            free(context_in_str);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32000));
            json_object_object_add(error_result, "message", json_object_new_string("Global RKNN context not initialized"));
            return error_result;
        }
        context_in = global_rknn_context;
    } else {
        // Parse as pointer string
        sscanf(context_in_str, "%p", (void**)&context_in);
    }
    free(context_in_str);
    
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