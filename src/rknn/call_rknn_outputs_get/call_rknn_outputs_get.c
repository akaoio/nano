#include "call_rknn_outputs_get.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../jsonrpc/extract_array_param/extract_array_param.h"
#include "../../jsonrpc/extract_bool_param/extract_bool_param.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_object_param/extract_object_param.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

json_object* call_rknn_outputs_get(json_object* params) {
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
    
    // Get n_outputs or num_outputs using jsonrpc function
    int n_outputs = extract_int_param(params, "n_outputs", 0);
    if (n_outputs <= 0) {
        n_outputs = extract_int_param(params, "num_outputs", 0);
    }
    if (n_outputs <= 0) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("n_outputs or num_outputs parameter is required and must be positive"));
        return error_result;
    }
    
    rknn_output* outputs = malloc(n_outputs * sizeof(rknn_output));
    if (!outputs) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Memory allocation failed"));
        return error_result;
    }
    
    // Parse outputs array if provided for preallocation using jsonrpc functions
    json_object* outputs_array = extract_array_param(params, "outputs");
    if (outputs_array) {
        int array_len = json_object_array_length(outputs_array);
        for (int i = 0; i < n_outputs && i < array_len; i++) {
            json_object* output_obj = json_object_array_get_idx(outputs_array, i);
            if (output_obj && json_object_is_type(output_obj, json_type_object)) {
                
                // Extract parameters using jsonrpc functions
                outputs[i].want_float = extract_bool_param(output_obj, "want_float", false) ? 1 : 0;
                outputs[i].is_prealloc = extract_bool_param(output_obj, "is_prealloc", false) ? 1 : 0;
                outputs[i].index = extract_int_param(output_obj, "index", i);
                
                // If prealloc, need buf and size
                if (outputs[i].is_prealloc) {
                    char* buf_str = extract_string_param(output_obj, "buf", NULL);
                    int size = extract_int_param(output_obj, "size", 0);
                    if (buf_str && size > 0) {
                        outputs[i].buf = (void*)buf_str; // Don't free as it's used by RKNN
                        outputs[i].size = size;
                    } else {
                        outputs[i].buf = NULL;
                        outputs[i].size = 0;
                    }
                } else {
                    outputs[i].buf = NULL;
                    outputs[i].size = 0;
                }
            } else {
                // Default values
                outputs[i].want_float = 0;
                outputs[i].is_prealloc = 0;
                outputs[i].index = i;
                outputs[i].buf = NULL;
                outputs[i].size = 0;
            }
        }
    } else {
        // Initialize with defaults
        for (int i = 0; i < n_outputs; i++) {
            outputs[i].want_float = 0;
            outputs[i].is_prealloc = 0;
            outputs[i].index = i;
            outputs[i].buf = NULL;
            outputs[i].size = 0;
        }
    }
    
    // Parse extend parameter if provided using jsonrpc functions
    rknn_output_extend extend = {0};
    json_object* extend_obj = extract_object_param(params, "extend");
    if (extend_obj) {
        extend.frame_id = extract_int_param(extend_obj, "frame_id", 0);
    }
    
    // Call RKNN function
    int ret = rknn_outputs_get(global_rknn_context, n_outputs, outputs, &extend);
    
    // Create result
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(ret == RKNN_SUCC));
    json_object_object_add(result, "ret_code", json_object_new_int(ret));
    
    if (ret == RKNN_SUCC) {
        // Create outputs array
        json_object* outputs_result = json_object_new_array();
        for (int i = 0; i < n_outputs; i++) {
            json_object* output_result = json_object_new_object();
            json_object_object_add(output_result, "index", json_object_new_int(outputs[i].index));
            json_object_object_add(output_result, "size", json_object_new_int(outputs[i].size));
            json_object_object_add(output_result, "want_float", json_object_new_boolean(outputs[i].want_float));
            json_object_object_add(output_result, "is_prealloc", json_object_new_boolean(outputs[i].is_prealloc));
            
            // Return summary of float data instead of full array for performance
            if (outputs[i].want_float && outputs[i].buf && outputs[i].size > 0) {
                int num_floats = outputs[i].size / sizeof(float);
                float* float_data = (float*)outputs[i].buf;
                
                json_object_object_add(output_result, "num_floats", json_object_new_int(num_floats));
                json_object_object_add(output_result, "data_available", json_object_new_boolean(true));
                
                // Check if full data is requested via extend parameter
                bool return_full_data = false;
                if (extend_obj) {
                    return_full_data = extract_bool_param(extend_obj, "return_full_data", false);
                }
                
                if (return_full_data && num_floats <= 10000) {
                    // Only return full array for small datasets to prevent crashes
                    json_object* full_array = json_object_new_array();
                    for (int j = 0; j < num_floats; j++) {
                        json_object_array_add(full_array, json_object_new_double(float_data[j]));
                    }
                    json_object_object_add(output_result, "data", full_array);
                } else if (return_full_data) {
                    // For large datasets, return as base64 encoded binary data
                    size_t binary_size = num_floats * sizeof(float);
                    
                    // Calculate base64 size (4/3 of binary size, rounded up)
                    size_t base64_size = ((binary_size + 2) / 3) * 4 + 1;
                    char* base64_data = malloc(base64_size);
                    
                    if (base64_data) {
                        // Simple base64 encoding (for production, use proper base64 encoder)
                        const char* b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
                        unsigned char* bytes = (unsigned char*)float_data;
                        size_t b64_idx = 0;
                        
                        for (size_t i = 0; i < binary_size; i += 3) {
                            unsigned int val = bytes[i] << 16;
                            if (i + 1 < binary_size) val |= bytes[i + 1] << 8;
                            if (i + 2 < binary_size) val |= bytes[i + 2];
                            
                            base64_data[b64_idx++] = b64chars[(val >> 18) & 0x3F];
                            base64_data[b64_idx++] = b64chars[(val >> 12) & 0x3F];
                            base64_data[b64_idx++] = (i + 1 < binary_size) ? b64chars[(val >> 6) & 0x3F] : '=';
                            base64_data[b64_idx++] = (i + 2 < binary_size) ? b64chars[val & 0x3F] : '=';
                        }
                        base64_data[b64_idx] = '\0';
                        
                        json_object_object_add(output_result, "data_base64", json_object_new_string(base64_data));
                        json_object_object_add(output_result, "data_format", json_object_new_string("float32_base64"));
                        free(base64_data);
                    }
                } else {
                    // Return preview for performance
                    json_object* preview_array = json_object_new_array();
                    int preview_count = num_floats < 10 ? num_floats : 10;
                    for (int j = 0; j < preview_count; j++) {
                        json_object_array_add(preview_array, json_object_new_double(float_data[j]));
                    }
                    json_object_object_add(output_result, "data_preview", preview_array);
                }
                
                // Store buffer pointer for potential future access
                char buf_ptr_str[32];
                snprintf(buf_ptr_str, sizeof(buf_ptr_str), "%p", outputs[i].buf);
                json_object_object_add(output_result, "buf_ptr", json_object_new_string(buf_ptr_str));
            } else {
                // For non-float data, return buffer pointer as string
                char buf_ptr_str[32];
                snprintf(buf_ptr_str, sizeof(buf_ptr_str), "%p", outputs[i].buf);
                json_object_object_add(output_result, "buf_ptr", json_object_new_string(buf_ptr_str));
            }
            
            json_object_array_add(outputs_result, output_result);
        }
        json_object_object_add(result, "outputs", outputs_result);
        
        // Add extend info
        json_object* extend_result = json_object_new_object();
        json_object_object_add(extend_result, "frame_id", json_object_new_int64(extend.frame_id));
        json_object_object_add(result, "extend", extend_result);
    } else {
        json_object_object_add(result, "error", json_object_new_string("rknn_outputs_get failed"));
    }
    
    free(outputs);
    return result;
}