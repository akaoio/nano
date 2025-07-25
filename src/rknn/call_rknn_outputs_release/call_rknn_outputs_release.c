#include "call_rknn_outputs_release.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../jsonrpc/extract_array_param/extract_array_param.h"
#include "../../jsonrpc/extract_bool_param/extract_bool_param.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_outputs_release(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters"));
        return error_result;
    }
    
    if (!global_rknn_initialized || global_rknn_context == 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("RKNN context not initialized"));
        return error_result;
    }
    
    int n_outputs = extract_int_param(params, "n_outputs", 0);
    if (n_outputs <= 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid n_outputs parameter"));
        return error_result;
    }
    
    json_object* outputs_array = extract_array_param(params, "outputs");
    if (!outputs_array) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Missing outputs array"));
        return error_result;
    }
    
    int array_len = json_object_array_length(outputs_array);
    if (array_len != n_outputs) {
        json_object_put(outputs_array);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Array length mismatch"));
        return error_result;
    }
    
    rknn_output* outputs = malloc(n_outputs * sizeof(rknn_output));
    if (!outputs) {
        json_object_put(outputs_array);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Memory allocation failed"));
        return error_result;
    }
    
    // Parse each output
    for (int i = 0; i < n_outputs; i++) {
        json_object* output_obj = json_object_array_get_idx(outputs_array, i);
        if (!output_obj || !json_object_is_type(output_obj, json_type_object)) {
            free(outputs);
            json_object_put(outputs_array);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32602));
            json_object_object_add(error_result, "message", json_object_new_string("Invalid output object"));
            return error_result;
        }
        
        // Extract parameters using jsonrpc functions
        outputs[i].index = extract_int_param(output_obj, "index", i);
        outputs[i].want_float = extract_bool_param(output_obj, "want_float", false) ? 1 : 0;
        outputs[i].is_prealloc = extract_bool_param(output_obj, "is_prealloc", false) ? 1 : 0;
        
        // Buffer pointer (from previous rknn_outputs_get call)
        char* buf_ptr_str = extract_string_param(output_obj, "buf_ptr", NULL);
        if (buf_ptr_str) {
            // Convert string pointer back to void*
            void* buf_ptr;
            sscanf(buf_ptr_str, "%p", &buf_ptr);
            outputs[i].buf = buf_ptr;
            free(buf_ptr_str);
        } else {
            outputs[i].buf = NULL;
        }
        
        outputs[i].size = extract_int_param(output_obj, "size", 0);
    }
    
    // Call RKNN function
    int ret = rknn_outputs_release(global_rknn_context, n_outputs, outputs);
    
    free(outputs);
    json_object_put(outputs_array);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_outputs_release failed"));
    }
    
    return result;
}