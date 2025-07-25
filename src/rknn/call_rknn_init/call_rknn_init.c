#include "call_rknn_init.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global state - only ONE vision model can be loaded at a time
rknn_context global_rknn_context = 0;
int global_rknn_initialized = 0;

json_object* call_rknn_init(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters"));
        return error_result;
    }
    
    // Only ONE model can be loaded at a time - destroy existing if needed
    if (global_rknn_initialized && global_rknn_context != 0) {
        rknn_destroy(global_rknn_context);
        global_rknn_context = 0;
        global_rknn_initialized = 0;
    }
    
    // Extract parameters using jsonrpc functions
    char* model_path = extract_string_param(params, "model_path", NULL);
    if (!model_path || strlen(model_path) == 0) {
        free(model_path);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("model_path parameter is required"));
        return error_result;
    }
    
    // Core mask is optional, default to 0 (auto)
    uint32_t core_mask = (uint32_t)extract_int_param(params, "core_mask", 0);
    
    // Read model file
    FILE* file = fopen(model_path, "rb");
    if (!file) {
        free(model_path);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Cannot open model file"));
        return error_result;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        free(model_path);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid model file size"));
        return error_result;
    }
    
    void* model_data = malloc(file_size);
    if (!model_data) {
        fclose(file);
        free(model_path);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Memory allocation failed"));
        return error_result;
    }
    
    size_t read_size = fread(model_data, 1, file_size, file);
    fclose(file);
    
    if (read_size != (size_t)file_size) {
        free(model_data);
        free(model_path);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Failed to read model file"));
        return error_result;
    }
    
    // Initialize RKNN context
    int ret = rknn_init(&global_rknn_context, model_data, file_size, 0, NULL);
    free(model_data);
    
    if (ret != RKNN_SUCC) {
        global_rknn_context = 0;
        free(model_path);
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("RKNN initialization failed"));
        return error_result;
    }
    
    // Set core mask if specified
    if (core_mask != 0) {
        ret = rknn_set_core_mask(global_rknn_context, core_mask);
        if (ret != RKNN_SUCC) {
            rknn_destroy(global_rknn_context);
            global_rknn_context = 0;
            free(model_path);
            json_object* error_result = json_object_new_object();
            json_object_object_add(error_result, "code", json_object_new_int(-32000));
            json_object_object_add(error_result, "message", json_object_new_string("Failed to set core mask"));
            return error_result;
        }
    }
    
    // Success - mark as initialized
    global_rknn_initialized = 1;
    
    // Return success result object
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string("Vision model initialized successfully"));
    json_object_object_add(result, "context", json_object_new_int64((int64_t)global_rknn_context));
    
    free(model_path);
    return result;
}