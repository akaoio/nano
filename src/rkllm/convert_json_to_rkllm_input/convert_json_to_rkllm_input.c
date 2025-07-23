#include "convert_json_to_rkllm_input.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

int convert_json_to_rkllm_input(json_object* json_obj, RKLLMInput* rkllm_input) {
    if (!json_obj || !rkllm_input) {
        return -1;
    }
    
    // Initialize structure to zero
    memset(rkllm_input, 0, sizeof(RKLLMInput));
    
    // Extract role (const char*)
    json_object* role_obj;
    if (json_object_object_get_ex(json_obj, "role", &role_obj)) {
        const char* role_str = json_object_get_string(role_obj);
        if (role_str) {
            rkllm_input->role = strdup(role_str);
        }
    }
    
    // Extract enable_thinking (bool)
    json_object* thinking_obj;
    if (json_object_object_get_ex(json_obj, "enable_thinking", &thinking_obj)) {
        rkllm_input->enable_thinking = json_object_get_boolean(thinking_obj);
    }
    
    // Extract input_type (RKLLMInputType enum)
    json_object* type_obj;
    if (json_object_object_get_ex(json_obj, "input_type", &type_obj)) {
        rkllm_input->input_type = json_object_get_int(type_obj);
    }
    
    // Handle union based on input_type
    switch (rkllm_input->input_type) {
        case RKLLM_INPUT_PROMPT: {
            // Extract prompt_input (const char*)
            json_object* prompt_obj;
            if (json_object_object_get_ex(json_obj, "prompt_input", &prompt_obj)) {
                const char* prompt_str = json_object_get_string(prompt_obj);
                if (prompt_str) {
                    rkllm_input->prompt_input = strdup(prompt_str);
                }
            }
            break;
        }
        
        case RKLLM_INPUT_TOKEN: {
            // Extract token_input (RKLLMTokenInput)
            json_object* token_obj;
            if (json_object_object_get_ex(json_obj, "token_input", &token_obj)) {
                // Extract input_ids array
                json_object* ids_obj;
                if (json_object_object_get_ex(token_obj, "input_ids", &ids_obj)) {
                    if (json_object_is_type(ids_obj, json_type_array)) {
                        int array_len = json_object_array_length(ids_obj);
                        if (array_len > 0) {
                            rkllm_input->token_input.input_ids = (int32_t*)malloc(array_len * sizeof(int32_t));
                            rkllm_input->token_input.n_tokens = array_len;
                            
                            for (int i = 0; i < array_len; i++) {
                                json_object* id_obj = json_object_array_get_idx(ids_obj, i);
                                rkllm_input->token_input.input_ids[i] = json_object_get_int(id_obj);
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case RKLLM_INPUT_EMBED: {
            // Extract embed_input (RKLLMEmbedInput)
            json_object* embed_obj;
            if (json_object_object_get_ex(json_obj, "embed_input", &embed_obj)) {
                // Extract embed array
                json_object* embed_array_obj;
                if (json_object_object_get_ex(embed_obj, "embed", &embed_array_obj)) {
                    if (json_object_is_type(embed_array_obj, json_type_array)) {
                        int array_len = json_object_array_length(embed_array_obj);
                        if (array_len > 0) {
                            rkllm_input->embed_input.embed = (float*)malloc(array_len * sizeof(float));
                            rkllm_input->embed_input.n_tokens = array_len;
                            
                            for (int i = 0; i < array_len; i++) {
                                json_object* embed_val_obj = json_object_array_get_idx(embed_array_obj, i);
                                rkllm_input->embed_input.embed[i] = json_object_get_double(embed_val_obj);
                            }
                        }
                    }
                }
            }
            break;
        }
        
        case RKLLM_INPUT_MULTIMODAL: {
            // Extract multimodal_input (RKLLMMultiModelInput)
            json_object* mm_obj;
            if (json_object_object_get_ex(json_obj, "multimodal_input", &mm_obj)) {
                // Extract prompt
                json_object* prompt_obj;
                if (json_object_object_get_ex(mm_obj, "prompt", &prompt_obj)) {
                    const char* prompt_str = json_object_get_string(prompt_obj);
                    if (prompt_str) {
                        rkllm_input->multimodal_input.prompt = strdup(prompt_str);
                    }
                }
                
                // Extract image_embed array
                json_object* img_embed_obj;
                if (json_object_object_get_ex(mm_obj, "image_embed", &img_embed_obj)) {
                    if (json_object_is_type(img_embed_obj, json_type_array)) {
                        int array_len = json_object_array_length(img_embed_obj);
                        if (array_len > 0) {
                            rkllm_input->multimodal_input.image_embed = (float*)malloc(array_len * sizeof(float));
                            
                            for (int i = 0; i < array_len; i++) {
                                json_object* embed_val_obj = json_object_array_get_idx(img_embed_obj, i);
                                rkllm_input->multimodal_input.image_embed[i] = json_object_get_double(embed_val_obj);
                            }
                        }
                    }
                }
                
                // Extract n_image_tokens
                json_object* n_img_tokens_obj;
                if (json_object_object_get_ex(mm_obj, "n_image_tokens", &n_img_tokens_obj)) {
                    rkllm_input->multimodal_input.n_image_tokens = json_object_get_int64(n_img_tokens_obj);
                }
                
                // Extract n_image
                json_object* n_image_obj;
                if (json_object_object_get_ex(mm_obj, "n_image", &n_image_obj)) {
                    rkllm_input->multimodal_input.n_image = json_object_get_int64(n_image_obj);
                }
                
                // Extract image_width
                json_object* img_width_obj;
                if (json_object_object_get_ex(mm_obj, "image_width", &img_width_obj)) {
                    rkllm_input->multimodal_input.image_width = json_object_get_int64(img_width_obj);
                }
                
                // Extract image_height
                json_object* img_height_obj;
                if (json_object_object_get_ex(mm_obj, "image_height", &img_height_obj)) {
                    rkllm_input->multimodal_input.image_height = json_object_get_int64(img_height_obj);
                }
            }
            break;
        }
        
        default:
            // Unknown input type - default to prompt
            rkllm_input->input_type = RKLLM_INPUT_PROMPT;
            break;
    }
    
    return 0;
}