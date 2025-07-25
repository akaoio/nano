#include "call_rkllm_run.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include "../manage_streaming_context/manage_streaming_context.h"
#include "../../jsonrpc/extract_string_param/extract_string_param.h"
#include "../../jsonrpc/extract_int_param/extract_int_param.h"
#include "../../jsonrpc/extract_object_param/extract_object_param.h"
#include "../../jsonrpc/extract_array_param/extract_array_param.h"
#include "../../utils/log_message/log_message.h"
#include "../../utils/base64_decode.h"
#include <stdbool.h>
#include <stdio.h>
#include <rkllm.h>
#include <string.h>
#include <stdlib.h>

// External reference to global LLM handle from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_run(json_object* params, int client_fd, int request_id) {
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("Model not initialized - call rkllm.init first"));
        return error_result;
    }
    
    if (!params || !json_object_is_type(params, json_type_object)) {
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32602));
        json_object_object_add(error_result, "message", json_object_new_string("Invalid parameters - expected object"));
        return error_result;
    }
    
    // Use params directly as the input object - standardized format
    json_object* input_obj = params;
    
    RKLLMInput rkllm_input;
    memset(&rkllm_input, 0, sizeof(RKLLMInput));
    
    // Extract input_type (support both string and int)
    json_object* input_type_obj = NULL;
    if (json_object_object_get_ex(input_obj, "input_type", &input_type_obj)) {
        if (json_object_is_type(input_type_obj, json_type_string)) {
            const char* input_type_str = json_object_get_string(input_type_obj);
            if (strcmp(input_type_str, "RKLLM_INPUT_MULTIMODAL") == 0) {
                rkllm_input.input_type = RKLLM_INPUT_MULTIMODAL;
            } else if (strcmp(input_type_str, "RKLLM_INPUT_PROMPT") == 0) {
                rkllm_input.input_type = RKLLM_INPUT_PROMPT;
            } else if (strcmp(input_type_str, "RKLLM_INPUT_TOKEN") == 0) {
                rkllm_input.input_type = RKLLM_INPUT_TOKEN;
            } else if (strcmp(input_type_str, "RKLLM_INPUT_EMBED") == 0) {
                rkllm_input.input_type = RKLLM_INPUT_EMBED;
            } else {
                rkllm_input.input_type = RKLLM_INPUT_PROMPT; // default
            }
        } else {
            rkllm_input.input_type = json_object_get_int(input_type_obj);
        }
    } else {
        rkllm_input.input_type = RKLLM_INPUT_PROMPT; // default
    }
    
    // Extract role
    char* role = extract_string_param(input_obj, "role", "user");
    if (role) {
        rkllm_input.role = role;  // Don't free - RKLLM keeps reference
    }
    
    // Handle union based on input_type
    switch (rkllm_input.input_type) {
        case RKLLM_INPUT_PROMPT: {
            char* prompt = extract_string_param(input_obj, "prompt", NULL);
            if (!prompt) {
                prompt = extract_string_param(input_obj, "prompt_input", NULL);
            }
            if (prompt) {
                rkllm_input.prompt_input = prompt;  // Don't free - RKLLM keeps reference
            }
            break;
        }
        case RKLLM_INPUT_MULTIMODAL: {
            // Handle multimodal input
            char* prompt = extract_string_param(input_obj, "prompt", NULL);
            if (prompt) {
                rkllm_input.prompt_input = prompt;
            }
            
            // Extract multimodal parameters
            json_object* multimodal_obj = extract_object_param(input_obj, "multimodal");
            if (multimodal_obj) {
                // Extract n_image_tokens FIRST to know expected size
                int n_image_tokens = extract_int_param(multimodal_obj, "n_image_tokens", 196);
                int embed_size = 1536;
                int expected_embed_len = n_image_tokens * embed_size;
                
                LOG_INFO_MSG("Expected multimodal embeddings: %d tokens × %d dim = %d floats", 
                           n_image_tokens, embed_size, expected_embed_len);
                
                // Try to extract image embeddings - support both array and base64 formats
                json_object* image_embed_array = extract_array_param(multimodal_obj, "image_embed");
                json_object* image_embed_base64_obj = NULL;
                json_object_object_get_ex(multimodal_obj, "image_embed_base64", &image_embed_base64_obj);
                
                float* embeddings = malloc(expected_embed_len * sizeof(float));
                if (!embeddings) {
                    LOG_ERROR_MSG("Failed to allocate memory for embeddings");
                } else {
                    bool embeddings_loaded = false;
                    
                    // Try base64 format first (more efficient for large arrays)
                    if (image_embed_base64_obj && json_object_is_type(image_embed_base64_obj, json_type_string)) {
                        const char* base64_data = json_object_get_string(image_embed_base64_obj);
                        if (base64_data) {
                            // Decode base64 to binary float data
                            unsigned char* decoded_data = NULL;
                            size_t decoded_len = 0;
                            
                            if (base64_decode(base64_data, &decoded_data, &decoded_len) == 0) {
                                size_t expected_bytes = expected_embed_len * sizeof(float);
                                size_t copy_bytes = (decoded_len < expected_bytes) ? decoded_len : expected_bytes;
                                
                                memcpy(embeddings, decoded_data, copy_bytes);
                                // Zero-pad if needed
                                if (copy_bytes < expected_bytes) {
                                    memset((char*)embeddings + copy_bytes, 0, expected_bytes - copy_bytes);
                                }
                                
                                free(decoded_data);
                                embeddings_loaded = true;
                                LOG_INFO_MSG("Loaded embeddings from base64: %zu bytes -> %d floats", 
                                           decoded_len, expected_embed_len);
                            } else {
                                LOG_ERROR_MSG("Failed to decode base64 embedding data");
                                free(decoded_data);
                            }
                        }
                    }
                    
                    // Fallback to JSON array format
                    if (!embeddings_loaded && image_embed_array) {
                        int embed_len = json_object_array_length(image_embed_array);
                        LOG_INFO_MSG("Received embedding array length: %d", embed_len);
                        
                        if (embed_len > 0) {
                            // Copy available embeddings, pad with zeros if needed
                            int copy_len = (embed_len < expected_embed_len) ? embed_len : expected_embed_len;
                            for (int i = 0; i < copy_len; i++) {
                                json_object* embed_val = json_object_array_get_idx(image_embed_array, i);
                                embeddings[i] = json_object_get_double(embed_val);
                            }
                            // Zero-pad if array is smaller than expected
                            for (int i = copy_len; i < expected_embed_len; i++) {
                                embeddings[i] = 0.0f;
                            }
                            embeddings_loaded = true;
                            LOG_INFO_MSG("Loaded embeddings from JSON array: %d floats", copy_len);
                        }
                    }
                    
                    if (embeddings_loaded) {
                        rkllm_input.multimodal_input.image_embed = embeddings;
                        rkllm_input.multimodal_input.n_image_tokens = n_image_tokens;
                        
                        LOG_INFO_MSG("Set multimodal embeddings: %d tokens, %d total floats", 
                                   n_image_tokens, expected_embed_len);
                    } else {
                        LOG_ERROR_MSG("No valid embedding data found");
                        free(embeddings);
                        embeddings = NULL;
                    }
                }
                
                // Extract other multimodal parameters
                rkllm_input.multimodal_input.n_image = extract_int_param(multimodal_obj, "n_image", 1);
                rkllm_input.multimodal_input.image_height = extract_int_param(multimodal_obj, "image_height", 392);
                rkllm_input.multimodal_input.image_width = extract_int_param(multimodal_obj, "image_width", 392);
                
                // CRITICAL: Log what we're actually sending to RKLLM
                LOG_INFO_MSG("MULTIMODAL PARAMS: n_image_tokens=%d, n_image=%d, height=%d, width=%d, embeddings=%p",
                           rkllm_input.multimodal_input.n_image_tokens,
                           rkllm_input.multimodal_input.n_image,
                           rkllm_input.multimodal_input.image_height,
                           rkllm_input.multimodal_input.image_width,
                           rkllm_input.multimodal_input.image_embed);
            }
            break;
        }
        // Add other input types as needed
    }
    
    // Use input_obj (params) for inference parameters as well - standardized format
    json_object* infer_obj = input_obj;  // Same as params, consistent with object format
    
    RKLLMInferParam rkllm_infer_param;
    memset(&rkllm_infer_param, 0, sizeof(RKLLMInferParam));
    
    // Extract mode
    rkllm_infer_param.mode = extract_int_param(infer_obj, "mode", 0);
    rkllm_infer_param.keep_history = extract_int_param(infer_obj, "keep_history", 0);
    
    // CRITICAL FIX: Handle different inference modes with proper validation
    LOG_INFO_MSG("Inference mode: %d", rkllm_infer_param.mode);
    
    // Mode-specific validation and warnings
    switch (rkllm_infer_param.mode) {
        case 0: // RKLLM_INFER_GENERATE
            LOG_INFO_MSG("Running text generation mode");
            break;
        case 1: // RKLLM_INFER_GET_LAST_HIDDEN_LAYER
            LOG_INFO_MSG("Running hidden states extraction mode");
            break;
        case 2: // RKLLM_INFER_GET_LOGITS
            LOG_INFO_MSG("Running logits extraction mode - checking RKLLM compatibility");
            break;
        default:
            LOG_WARN_MSG("Unknown inference mode: %d", rkllm_infer_param.mode);
            break;
    }
    
    // Set streaming context for the callback to capture streaming data
    set_streaming_context(client_fd, request_id);
    LOG_DEBUG_MSG("Set streaming context for rkllm_run (mode: %d)", rkllm_infer_param.mode);
    
    // Call rkllm_run - the callback will handle ALL responses including final
    LOG_INFO_MSG("Calling rkllm_run...");
    int result = rkllm_run(global_llm_handle, &rkllm_input, &rkllm_infer_param, NULL);
    
    LOG_INFO_MSG("rkllm_run returned: %d", result);
    
    if (result != 0) {
        // RKLLM run failed - cleanup and return error
        LOG_ERROR_MSG("rkllm_run failed with code: %d", result);
        clear_streaming_context();
        
        // Cleanup allocated memory
        if (rkllm_input.input_type == RKLLM_INPUT_MULTIMODAL && rkllm_input.multimodal_input.image_embed) {
            free((void*)rkllm_input.multimodal_input.image_embed);
        }
        
        json_object* error_result = json_object_new_object();
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string("RKLLM run failed"));
        return error_result;
    }
    
    // Cleanup allocated memory
    if (rkllm_input.input_type == RKLLM_INPUT_MULTIMODAL && rkllm_input.multimodal_input.image_embed) {
        free((void*)rkllm_input.multimodal_input.image_embed);
    }
    
    // CRITICAL FIX: For async mode, return NULL to indicate "no immediate response"
    // The callback function will handle ALL responses to the client
    LOG_DEBUG_MSG("Async mode: returning NULL (callback handles responses)");
    return NULL; // No immediate response - callback handles everything
}