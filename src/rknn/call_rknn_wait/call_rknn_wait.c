#include "call_rknn_wait.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_wait(json_object* params) {
    if (!global_rknn_initialized || global_rknn_context == 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "success", json_object_new_boolean(0));
        json_object_object_add(error_result, "error", json_object_new_string("RKNN context not initialized"));
        return error_result;
    }
    
    rknn_run_extend extend = {0};
    
    // Parse extend parameter if provided
    if (params && json_object_is_type(params, json_type_object)) {
        json_object* extend_obj;
        if (json_object_object_get_ex(params, "extend", &extend_obj) && 
            json_object_is_type(extend_obj, json_type_object)) {
            
            json_object* frame_id_obj;
            if (json_object_object_get_ex(extend_obj, "frame_id", &frame_id_obj)) {
                extend.frame_id = json_object_get_int64(frame_id_obj);
            }
            
            json_object* non_block_obj;
            if (json_object_object_get_ex(extend_obj, "non_block", &non_block_obj)) {
                extend.non_block = json_object_get_int(non_block_obj);
            }
            
            json_object* timeout_ms_obj;
            if (json_object_object_get_ex(extend_obj, "timeout_ms", &timeout_ms_obj)) {
                extend.timeout_ms = json_object_get_int(timeout_ms_obj);
            }
            
            json_object* fence_fd_obj;
            if (json_object_object_get_ex(extend_obj, "fence_fd", &fence_fd_obj)) {
                extend.fence_fd = json_object_get_int(fence_fd_obj);
            }
        }
    }
    
    // Call RKNN function
    int ret = rknn_wait(global_rknn_context, &extend);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret == RKNN_SUCC) {
        json_object* extend_result = json_object_new_object();
        json_object_object_add(extend_result, "frame_id", json_object_new_int64(extend.frame_id));
        json_object_object_add(result, "extend", extend_result);
    } else {
        json_object_object_add(result, "error", json_object_new_string("rknn_wait failed"));
    }
    
    return result;
}