#include "convert_json_to_rkllm_infer_param.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

int convert_json_to_rkllm_infer_param(json_object* json_obj, RKLLMInferParam* rkllm_infer_param) {
    if (!json_obj || !rkllm_infer_param) {
        return -1;
    }
    
    // Initialize structure to zero
    memset(rkllm_infer_param, 0, sizeof(RKLLMInferParam));
    
    // Extract mode (RKLLMInferMode enum)
    json_object* mode_obj;
    if (json_object_object_get_ex(json_obj, "mode", &mode_obj)) {
        rkllm_infer_param->mode = json_object_get_int(mode_obj);
    }
    
    // Extract lora_params (RKLLMLoraParam* or null)
    json_object* lora_obj;
    if (json_object_object_get_ex(json_obj, "lora_params", &lora_obj)) {
        if (!json_object_is_type(lora_obj, json_type_null)) {
            // Allocate and fill lora_params
            rkllm_infer_param->lora_params = (RKLLMLoraParam*)malloc(sizeof(RKLLMLoraParam));
            memset(rkllm_infer_param->lora_params, 0, sizeof(RKLLMLoraParam));
            
            // Extract lora_adapter_name
            json_object* adapter_name_obj;
            if (json_object_object_get_ex(lora_obj, "lora_adapter_name", &adapter_name_obj)) {
                const char* adapter_name_str = json_object_get_string(adapter_name_obj);
                if (adapter_name_str) {
                    rkllm_infer_param->lora_params->lora_adapter_name = strdup(adapter_name_str);
                }
            }
        }
    }
    
    // Extract prompt_cache_params (RKLLMPromptCacheParam* or null)
    json_object* cache_obj;
    if (json_object_object_get_ex(json_obj, "prompt_cache_params", &cache_obj)) {
        if (!json_object_is_type(cache_obj, json_type_null)) {
            // Allocate and fill prompt_cache_params
            rkllm_infer_param->prompt_cache_params = (RKLLMPromptCacheParam*)malloc(sizeof(RKLLMPromptCacheParam));
            memset(rkllm_infer_param->prompt_cache_params, 0, sizeof(RKLLMPromptCacheParam));
            
            // Extract save_prompt_cache
            json_object* save_cache_obj;
            if (json_object_object_get_ex(cache_obj, "save_prompt_cache", &save_cache_obj)) {
                rkllm_infer_param->prompt_cache_params->save_prompt_cache = json_object_get_int(save_cache_obj);
            }
            
            // Extract prompt_cache_path
            json_object* cache_path_obj;
            if (json_object_object_get_ex(cache_obj, "prompt_cache_path", &cache_path_obj)) {
                const char* cache_path_str = json_object_get_string(cache_path_obj);
                if (cache_path_str) {
                    rkllm_infer_param->prompt_cache_params->prompt_cache_path = strdup(cache_path_str);
                }
            }
        }
    }
    
    // Extract keep_history (int)
    json_object* history_obj;
    if (json_object_object_get_ex(json_obj, "keep_history", &history_obj)) {
        rkllm_infer_param->keep_history = json_object_get_int(history_obj);
    }
    
    return 0;
}