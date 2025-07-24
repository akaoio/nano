#include "call_rknn_query.h"
#include "../call_rknn_init/call_rknn_init.h"
#include <stdio.h>
#include <string.h>

json_object* call_rknn_query(json_object* params) {
    if (!global_rknn_initialized || global_rknn_context == 0) {
        return NULL;
    }
    
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
    }
    
    // Extract query type parameter
    json_object* query_type_obj = NULL;
    if (!json_object_object_get_ex(params, "query_type", &query_type_obj)) {
        // Default to SDK version query
        query_type_obj = json_object_new_string("sdk_version");
    }
    
    const char* query_type_str = json_object_get_string(query_type_obj);
    if (!query_type_str) {
        return NULL;
    }
    
    json_object* result = json_object_new_object();
    
    // Handle different query types
    if (strcmp(query_type_str, "sdk_version") == 0) {
        rknn_sdk_version version;
        int ret = rknn_query(global_rknn_context, RKNN_QUERY_SDK_VERSION, &version, sizeof(version));
        if (ret == RKNN_SUCC) {
            json_object_object_add(result, "api_version", json_object_new_string(version.api_version));
            json_object_object_add(result, "driver_version", json_object_new_string(version.drv_version));
        }
    } 
    else if (strcmp(query_type_str, "input_attr") == 0) {
        rknn_input_output_num io_num;
        int ret = rknn_query(global_rknn_context, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
        if (ret == RKNN_SUCC) {
            json_object* inputs_array = json_object_new_array();
            
            for (uint32_t i = 0; i < io_num.n_input; i++) {
                rknn_tensor_attr input_attr;
                input_attr.index = i;
                ret = rknn_query(global_rknn_context, RKNN_QUERY_INPUT_ATTR, &input_attr, sizeof(input_attr));
                if (ret == RKNN_SUCC) {
                    json_object* input_obj = json_object_new_object();
                    json_object_object_add(input_obj, "index", json_object_new_int(input_attr.index));
                    json_object_object_add(input_obj, "name", json_object_new_string(input_attr.name));
                    json_object_object_add(input_obj, "n_dims", json_object_new_int(input_attr.n_dims));
                    
                    json_object* dims_array = json_object_new_array();
                    for (uint32_t j = 0; j < input_attr.n_dims; j++) {
                        json_object_array_add(dims_array, json_object_new_int(input_attr.dims[j]));
                    }
                    json_object_object_add(input_obj, "dims", dims_array);
                    json_object_object_add(input_obj, "size", json_object_new_int(input_attr.size));
                    json_object_object_add(input_obj, "fmt", json_object_new_int(input_attr.fmt));
                    json_object_object_add(input_obj, "type", json_object_new_int(input_attr.type));
                    json_object_object_add(input_obj, "qnt_type", json_object_new_int(input_attr.qnt_type));
                    
                    json_object_array_add(inputs_array, input_obj);
                }
            }
            json_object_object_add(result, "inputs", inputs_array);
        }
    }
    else if (strcmp(query_type_str, "output_attr") == 0) {
        rknn_input_output_num io_num;
        int ret = rknn_query(global_rknn_context, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
        if (ret == RKNN_SUCC) {
            json_object* outputs_array = json_object_new_array();
            
            for (uint32_t i = 0; i < io_num.n_output; i++) {
                rknn_tensor_attr output_attr;
                output_attr.index = i;
                ret = rknn_query(global_rknn_context, RKNN_QUERY_OUTPUT_ATTR, &output_attr, sizeof(output_attr));
                if (ret == RKNN_SUCC) {
                    json_object* output_obj = json_object_new_object();
                    json_object_object_add(output_obj, "index", json_object_new_int(output_attr.index));
                    json_object_object_add(output_obj, "name", json_object_new_string(output_attr.name));
                    json_object_object_add(output_obj, "n_dims", json_object_new_int(output_attr.n_dims));
                    
                    json_object* dims_array = json_object_new_array();
                    for (uint32_t j = 0; j < output_attr.n_dims; j++) {
                        json_object_array_add(dims_array, json_object_new_int(output_attr.dims[j]));
                    }
                    json_object_object_add(output_obj, "dims", dims_array);
                    json_object_object_add(output_obj, "size", json_object_new_int(output_attr.size));
                    json_object_object_add(output_obj, "fmt", json_object_new_int(output_attr.fmt));
                    json_object_object_add(output_obj, "type", json_object_new_int(output_attr.type));
                    json_object_object_add(output_obj, "qnt_type", json_object_new_int(output_attr.qnt_type));
                    
                    json_object_array_add(outputs_array, output_obj);
                }
            }
            json_object_object_add(result, "outputs", outputs_array);
        }
    }
    
    json_object_object_add(result, "success", json_object_new_boolean(1));
    return result;
}