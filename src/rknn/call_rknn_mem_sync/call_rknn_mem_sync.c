#include "call_rknn_mem_sync.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_mem_sync(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
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
    
    // Get mem parameter using jsonrpc function
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
    
    // Get sync_type parameter using jsonrpc function
    int sync_type_int = extract_int_param(params, "sync_type", 0);
    rknn_mem_sync_mode sync_type = (rknn_mem_sync_mode)sync_type_int;
    
    // Call RKNN function
    int ret = rknn_mem_sync(context, mem, sync_type);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_mem_sync failed"));
    }
    
    return result;
}