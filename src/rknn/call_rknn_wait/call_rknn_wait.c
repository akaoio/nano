#include "call_rknn_wait.h"
#include "../../jsonrpc/extract_object_param/extract_object_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_wait(json_object* params) {
    if (!global_rknn_initialized || global_rknn_context == 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("RKNN context not initialized"));
        return error_result;
    }
    
    rknn_run_extend extend = {0};
    
    // Parse extend parameter if provided using jsonrpc functions
    if (params && json_object_is_type(params, json_type_object)) {
        json_object* extend_obj = extract_object_param(params, "extend");
        if (extend_obj) {
            extend.frame_id = extract_int_param(extend_obj, "frame_id", 0);
            extend.non_block = extract_int_param(extend_obj, "non_block", 0);
            extend.timeout_ms = extract_int_param(extend_obj, "timeout_ms", 0);
            
            extend.fence_fd = extract_int_param(extend_obj, "fence_fd", 0);
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