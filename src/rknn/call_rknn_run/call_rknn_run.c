#include "call_rknn_run.h"
#include "../call_rknn_init/call_rknn_init.h"
#include <rknn_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

json_object* call_rknn_run(json_object* params) {
    if (!global_rknn_initialized || global_rknn_context == 0) {
        return NULL;
    }
    
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
    }
    
    // For now, this is a basic implementation that just runs inference
    // In a full implementation, this would handle setting inputs from params
    
    // Query input/output attributes
    rknn_input_output_num io_num;
    int ret = rknn_query(global_rknn_context, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC) {
        return NULL;
    }
    
    // Allocate input tensors (basic implementation)
    rknn_input* inputs = calloc(io_num.n_input, sizeof(rknn_input));
    if (!inputs) {
        return NULL;
    }
    
    // Set up input tensors (simplified - would need proper data from params)
    for (uint32_t i = 0; i < io_num.n_input; i++) {
        rknn_tensor_attr input_attr;
        input_attr.index = i;
        ret = rknn_query(global_rknn_context, RKNN_QUERY_INPUT_ATTR, &input_attr, sizeof(input_attr));
        if (ret != RKNN_SUCC) {
            free(inputs);
            return NULL;
        }
        
        inputs[i].index = i;
        inputs[i].buf = malloc(input_attr.size);
        inputs[i].size = input_attr.size;
        inputs[i].pass_through = 0;
        inputs[i].type = input_attr.type;
        inputs[i].fmt = input_attr.fmt;
        
        if (!inputs[i].buf) {
            for (uint32_t j = 0; j < i; j++) {
                free(inputs[j].buf);
            }
            free(inputs);
            return NULL;
        }
        
        // Initialize with zeros (would use real data from params)
        memset(inputs[i].buf, 0, input_attr.size);
    }
    
    // Set inputs
    ret = rknn_inputs_set(global_rknn_context, io_num.n_input, inputs);
    if (ret != RKNN_SUCC) {
        for (uint32_t i = 0; i < io_num.n_input; i++) {
            free(inputs[i].buf);
        }
        free(inputs);
        return NULL;
    }
    
    // Run inference
    ret = rknn_run(global_rknn_context, NULL);
    if (ret != RKNN_SUCC) {
        for (uint32_t i = 0; i < io_num.n_input; i++) {
            free(inputs[i].buf);
        }
        free(inputs);
        return NULL;
    }
    
    // Get outputs
    rknn_output* outputs = calloc(io_num.n_output, sizeof(rknn_output));
    if (!outputs) {
        for (uint32_t i = 0; i < io_num.n_input; i++) {
            free(inputs[i].buf);
        }
        free(inputs);
        return NULL;
    }
    
    ret = rknn_outputs_get(global_rknn_context, io_num.n_output, outputs, NULL);
    if (ret != RKNN_SUCC) {
        free(outputs);
        for (uint32_t i = 0; i < io_num.n_input; i++) {
            free(inputs[i].buf);
        }
        free(inputs);
        return NULL;
    }
    
    // Create result object with output information
    json_object* result = json_object_new_object();
    json_object* outputs_array = json_object_new_array();
    
    for (uint32_t i = 0; i < io_num.n_output; i++) {
        json_object* output_obj = json_object_new_object();
        json_object_object_add(output_obj, "index", json_object_new_int(i));
        json_object_object_add(output_obj, "size", json_object_new_int(outputs[i].size));
        json_object_object_add(output_obj, "want_float", json_object_new_boolean(outputs[i].want_float));
        json_object_object_add(output_obj, "is_prealloc", json_object_new_boolean(outputs[i].is_prealloc));
        
        // Note: Not including actual data buffer for now - would be too large for JSON
        // In a real implementation, this would serialize the output data appropriately
        
        json_object_array_add(outputs_array, output_obj);
    }
    
    json_object_object_add(result, "outputs", outputs_array);
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string("Inference completed successfully"));
    
    // Release outputs
    rknn_outputs_release(global_rknn_context, io_num.n_output, outputs);
    free(outputs);
    
    // Clean up inputs
    for (uint32_t i = 0; i < io_num.n_input; i++) {
        free(inputs[i].buf);
    }
    free(inputs);
    
    return result;
}