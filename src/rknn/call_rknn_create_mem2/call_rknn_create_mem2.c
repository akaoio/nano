#include "call_rknn_create_mem2.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_create_mem2(json_object* params) {
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
    
    // Get size parameter using jsonrpc function
    int size_int = extract_int_param(params, "size", 0);
    if (size_int <= 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("size parameter is required and must be positive"));
        return error_result;
    }
    uint64_t size = (uint64_t)size_int;
    
    // Get flags parameter using jsonrpc function (default to 0)
    int flags_int = extract_int_param(params, "flags", 0);
    uint64_t flags = (uint64_t)flags_int;
    
    // Call RKNN function
    rknn_tensor_mem* mem = rknn_create_mem2(context, size, flags);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(mem != NULL));
    
    if (mem) {
        // Return memory information
        json_object* mem_obj = json_object_new_object();
        
        char virt_addr_str[32];
        snprintf(virt_addr_str, sizeof(virt_addr_str), "%p", mem->virt_addr);
        json_object_object_add(mem_obj, "virt_addr", json_object_new_string(virt_addr_str));
        json_object_object_add(mem_obj, "phys_addr", json_object_new_int64(mem->phys_addr));
        json_object_object_add(mem_obj, "fd", json_object_new_int(mem->fd));
        json_object_object_add(mem_obj, "offset", json_object_new_int(mem->offset));
        json_object_object_add(mem_obj, "size", json_object_new_int(mem->size));
        json_object_object_add(mem_obj, "flags", json_object_new_int(mem->flags));
        
        char priv_data_str[32];
        snprintf(priv_data_str, sizeof(priv_data_str), "%p", mem->priv_data);
        json_object_object_add(mem_obj, "priv_data", json_object_new_string(priv_data_str));
        
        // Return pointer to memory structure itself
        char mem_ptr_str[32];
        snprintf(mem_ptr_str, sizeof(mem_ptr_str), "%p", mem);
        json_object_object_add(mem_obj, "mem_ptr", json_object_new_string(mem_ptr_str));
        
        json_object_object_add(result, "memory", mem_obj);
    } else {
        json_object_object_add(result, "error", json_object_new_string("rknn_create_mem2 failed"));
    }
    
    return result;
}