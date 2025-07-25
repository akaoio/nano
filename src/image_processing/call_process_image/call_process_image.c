#include "call_process_image.h"
#include "../process_image/process_image.h"
#include "../../utils/log_message/log_message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ImageProcessor* global_processor = NULL;

json_object* call_init_image_processor(json_object* params) {
    json_object* response = json_object_new_object();
    
    if (!params) {
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32602));
        json_object_object_add(error, "message", json_object_new_string("Missing parameters"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    // Get model_path parameter
    json_object* model_path_obj;
    if (!json_object_object_get_ex(params, "model_path", &model_path_obj)) {
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32602));
        json_object_object_add(error, "message", json_object_new_string("Missing model_path parameter"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    const char* model_path = json_object_get_string(model_path_obj);
    
    // Get optional core_num parameter (default to 1)
    int core_num = 1;
    json_object* core_num_obj;
    if (json_object_object_get_ex(params, "core_num", &core_num_obj)) {
        core_num = json_object_get_int(core_num_obj);
    }
    
    // Clean up existing processor if any
    if (global_processor) {
        cleanup_image_processor(global_processor);
        free(global_processor);
    }
    
    // Initialize new processor
    global_processor = (ImageProcessor*)malloc(sizeof(ImageProcessor));
    if (!global_processor) {
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32603));
        json_object_object_add(error, "message", json_object_new_string("Memory allocation failed"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    int ret = init_image_processor(global_processor, model_path, core_num);
    if (ret != 0) {
        free(global_processor);
        global_processor = NULL;
        
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32603));
        json_object_object_add(error, "message", json_object_new_string("Failed to initialize image processor"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    LOG_INFO_MSG("Image processor initialized with model: %s, cores: %d", model_path, core_num);
    
    json_object* result = json_object_new_object();
    json_object_object_add(result, "status", json_object_new_string("initialized"));
    json_object_object_add(result, "model_path", json_object_new_string(model_path));
    json_object_object_add(result, "core_num", json_object_new_int(core_num));
    json_object_object_add(response, "result", result);
    
    return response;
}

json_object* call_process_image(json_object* params) {
    json_object* response = json_object_new_object();
    
    if (!global_processor) {
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32603));
        json_object_object_add(error, "message", json_object_new_string("Image processor not initialized"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    if (!params) {
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32602));
        json_object_object_add(error, "message", json_object_new_string("Missing parameters"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    // Get image_data parameter (base64 encoded)
    json_object* image_data_obj;
    if (!json_object_object_get_ex(params, "image_data", &image_data_obj)) {
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32602));
        json_object_object_add(error, "message", json_object_new_string("Missing image_data parameter"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    const char* image_data = json_object_get_string(image_data_obj);
    
    // Allocate embeddings buffer
    size_t max_embedding_size = 196 * 1536; // Qwen2-VL dimensions
    float* embeddings = (float*)malloc(max_embedding_size * sizeof(float));
    if (!embeddings) {
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32603));
        json_object_object_add(error, "message", json_object_new_string("Memory allocation failed"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    size_t embedding_size;
    int ret = process_image_base64(global_processor, image_data, embeddings, &embedding_size);
    
    if (ret != 0) {
        free(embeddings);
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32603));
        json_object_object_add(error, "message", json_object_new_string("Image processing failed"));
        json_object_object_add(response, "error", error);
        return response;
    }
    
    // Convert embeddings to JSON array
    json_object* embeddings_array = json_object_new_array();
    for (size_t i = 0; i < embedding_size; i++) {
        json_object_array_add(embeddings_array, json_object_new_double(embeddings[i]));
    }
    
    json_object* result = json_object_new_object();
    json_object_object_add(result, "embeddings", embeddings_array);
    json_object_object_add(result, "embedding_size", json_object_new_int64(embedding_size));
    json_object_object_add(result, "n_image_tokens", json_object_new_int(196));
    json_object_object_add(result, "embed_dim", json_object_new_int(1536));
    json_object_object_add(response, "result", result);
    
    free(embeddings);
    LOG_INFO_MSG("Processed image, generated %zu embeddings", embedding_size);
    
    return response;
}

json_object* call_cleanup_image_processor(void) {
    json_object* response = json_object_new_object();
    
    if (global_processor) {
        cleanup_image_processor(global_processor);
        free(global_processor);
        global_processor = NULL;
        LOG_INFO_MSG("Image processor cleaned up");
    }
    
    json_object* result = json_object_new_object();
    json_object_object_add(result, "status", json_object_new_string("cleaned_up"));
    json_object_object_add(response, "result", result);
    
    return response;
}