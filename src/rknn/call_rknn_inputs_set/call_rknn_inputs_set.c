#include "call_rknn_inputs_set.h"
#include "../../jsonrpc/extract_array_param/extract_array_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_bool_param/extract_bool_param.h"
#include "../../utils/base64_decode.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_inputs_set(json_object* params) {
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
    
    // Get inputs array using jsonrpc function
    json_object* inputs_array = extract_array_param(params, "inputs");
    if (!inputs_array) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("inputs parameter is required and must be array"));
        return error_result;
    }
    
    int n_inputs = json_object_array_length(inputs_array);
    if (n_inputs == 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("inputs array cannot be empty"));
        return error_result;
    }
    
    rknn_input* inputs = malloc(n_inputs * sizeof(rknn_input));
    if (!inputs) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Memory allocation failed"));
        return error_result;
    }
    
    // Parse each input using jsonrpc functions
    for (int i = 0; i < n_inputs; i++) {
        json_object* input_obj = json_object_array_get_idx(inputs_array, i);
        if (!input_obj || !json_object_is_type(input_obj, json_type_object)) {
            free(inputs);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32602));
            json_object_object_add(error_result, "message", json_object_new_string("Invalid input object in array"));
            return error_result;
        }
        
        // Extract parameters using jsonrpc functions
        int index_val = extract_int_param(input_obj, "index", -1);
        if (index_val == -1) {
            free(inputs);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32602));
            json_object_object_add(error_result, "message", json_object_new_string("index parameter is required for each input"));
            return error_result;
        }
        inputs[i].index = (uint32_t)index_val;
        
        // Try both "data" and "buf" parameter names for compatibility
        char* data_str = extract_string_param(input_obj, "data", NULL);
        if (!data_str) {
            data_str = extract_string_param(input_obj, "buf", NULL);
        }
        if (!data_str) {
            free(inputs);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32602));
            json_object_object_add(error_result, "message", json_object_new_string("data or buf parameter is required for each input"));
            return error_result;
        }
        
        // Decode base64 data to binary format for RKNN
        unsigned char* decoded_data = NULL;
        size_t decoded_len = 0;
        
        if (base64_decode(data_str, &decoded_data, &decoded_len) != 0) {
            free(inputs);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32602));
            json_object_object_add(error_result, "message", json_object_new_string("Failed to decode base64 image data"));
            return error_result;
        }
        
        inputs[i].buf = (void*)decoded_data;
        
        // Use decoded length or provided size parameter
        int provided_size = extract_int_param(input_obj, "size", 0);
        if (provided_size > 0) {
            inputs[i].size = provided_size;
        } else {
            inputs[i].size = decoded_len;
        }
        
        if (inputs[i].size <= 0) {
            free(decoded_data);
            free(inputs);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32602));
            json_object_object_add(error_result, "message", json_object_new_string("Invalid data size"));
            return error_result;
        }
        
        inputs[i].pass_through = extract_bool_param(input_obj, "pass_through", false) ? 1 : 0;
        inputs[i].type = extract_int_param(input_obj, "type", RKNN_TENSOR_UINT8);
        inputs[i].fmt = extract_int_param(input_obj, "fmt", RKNN_TENSOR_NHWC);
    }
    
    // Call RKNN function
    int ret = rknn_inputs_set(global_rknn_context, n_inputs, inputs);
    
    // Clean up decoded data buffers
    for (int i = 0; i < n_inputs; i++) {
        if (inputs[i].buf) {
            free(inputs[i].buf);
        }
    }
    
    free(inputs);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret != RKNN_SUCC) {
        json_object_object_add(result, "error", json_object_new_string("rknn_inputs_set failed"));
    }
    
    return result;
}