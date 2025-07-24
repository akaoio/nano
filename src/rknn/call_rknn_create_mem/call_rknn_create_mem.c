#include "call_rknn_create_mem.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_create_mem(json_object* params) {
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
    
    // Get size parameter
    json_object* size_obj;
    if (!json_object_object_get_ex(params, "size", &size_obj)) {
        return NULL;
    }
    uint32_t size = json_object_get_int(size_obj);
    
    // Call RKNN function
    rknn_tensor_mem* mem = rknn_create_mem(context, size);
    
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
        json_object_object_add(result, "error", json_object_new_string("rknn_create_mem failed"));
    }
    
    return result;
}