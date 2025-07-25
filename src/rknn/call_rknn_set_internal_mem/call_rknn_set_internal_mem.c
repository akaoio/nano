#include "call_rknn_set_internal_mem.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_set_internal_mem(json_object* params) {
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
    
    // Get mem parameter (pointer string) using jsonrpc function
    char* mem_str = extract_string_param(params, "mem", NULL);
    if (!mem_str) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("mem parameter is required"));
        return error_result;
    }
    
    rknn_tensor_mem* mem;
    sscanf(mem_str, "%p", (void**)&mem);
    free(mem_str);
    
    // Call RKNN function
    int ret = rknn_set_internal_mem(context, mem);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_set_internal_mem failed"));
    }
    
    return result;
}