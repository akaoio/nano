#include "call_rknn_destroy.h"
#include "../call_rknn_init/call_rknn_init.h"
#include <rknn_api.h>

json_object* call_rknn_destroy(json_object* params) {
    (void)params; // Unused parameter
    
    if (global_rknn_initialized && global_rknn_context != 0) {
        int ret = rknn_destroy(global_rknn_context);
        global_rknn_context = 0;
        global_rknn_initialized = 0;
        
        if (ret != RKNN_SUCC) {
            return NULL;
        }
    }
    
    // Return success result object
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string("Vision model destroyed successfully"));
    
    return result;
}