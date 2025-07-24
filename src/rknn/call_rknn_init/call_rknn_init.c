#include "call_rknn_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global state - only ONE vision model can be loaded at a time
rknn_context global_rknn_context = 0;
int global_rknn_initialized = 0;

json_object* call_rknn_init(json_object* params) {
    if (!params || !json_object_is_type(params, json_type_object)) {
        return NULL;
    }
    
    // Only ONE model can be loaded at a time - destroy existing if needed
    if (global_rknn_initialized && global_rknn_context != 0) {
        rknn_destroy(global_rknn_context);
        global_rknn_context = 0;
        global_rknn_initialized = 0;
    }
    
    // Extract parameters
    json_object* model_path_obj = NULL;
    json_object* core_mask_obj = NULL;
    
    if (!json_object_object_get_ex(params, "model_path", &model_path_obj)) {
        return NULL;
    }
    
    const char* model_path = json_object_get_string(model_path_obj);
    if (!model_path || strlen(model_path) == 0) {
        return NULL;
    }
    
    // Core mask is optional, default to 0 (auto)
    uint32_t core_mask = 0;
    if (json_object_object_get_ex(params, "core_mask", &core_mask_obj)) {
        core_mask = json_object_get_int(core_mask_obj);
    }
    
    // Read model file
    FILE* file = fopen(model_path, "rb");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return NULL;
    }
    
    void* model_data = malloc(file_size);
    if (!model_data) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(model_data, 1, file_size, file);
    fclose(file);
    
    if (read_size != (size_t)file_size) {
        free(model_data);
        return NULL;
    }
    
    // Initialize RKNN context
    int ret = rknn_init(&global_rknn_context, model_data, file_size, 0, NULL);
    free(model_data);
    
    if (ret != RKNN_SUCC) {
        global_rknn_context = 0;
        return NULL;
    }
    
    // Set core mask if specified
    if (core_mask != 0) {
        ret = rknn_set_core_mask(global_rknn_context, core_mask);
        if (ret != RKNN_SUCC) {
            rknn_destroy(global_rknn_context);
            global_rknn_context = 0;
            return NULL;
        }
    }
    
    // Success - mark as initialized
    global_rknn_initialized = 1;
    
    // Return success result object
    json_object* result = json_object_new_object();
    json_object_object_add(result, "success", json_object_new_boolean(1));
    json_object_object_add(result, "message", json_object_new_string("Vision model initialized successfully"));
    json_object_object_add(result, "context", json_object_new_int64((int64_t)global_rknn_context));
    
    return result;
}