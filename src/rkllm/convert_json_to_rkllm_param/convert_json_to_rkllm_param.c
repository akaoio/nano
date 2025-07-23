#include "convert_json_to_rkllm_param.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

int convert_json_to_rkllm_param(json_object* json_param, RKLLMParam* param) {
    if (!json_param || !param) {
        return -1;
    }
    
    // Initialize with default values first
    *param = rkllm_createDefaultParam();
    
    // Extract and set each parameter with 1:1 mapping
    json_object* obj;
    
    // model_path (required)
    if (json_object_object_get_ex(json_param, "model_path", &obj)) {
        const char* model_path = json_object_get_string(obj);
        if (model_path && strlen(model_path) > 0) {
            param->model_path = strdup(model_path);
            if (!param->model_path) {
                return -1; // Memory allocation failed
            }
        }
    }
    
    // max_context_len
    if (json_object_object_get_ex(json_param, "max_context_len", &obj)) {
        param->max_context_len = json_object_get_int(obj);
    }
    
    // max_new_tokens
    if (json_object_object_get_ex(json_param, "max_new_tokens", &obj)) {
        param->max_new_tokens = json_object_get_int(obj);
    }
    
    // top_k
    if (json_object_object_get_ex(json_param, "top_k", &obj)) {
        param->top_k = json_object_get_int(obj);
    }
    
    // n_keep
    if (json_object_object_get_ex(json_param, "n_keep", &obj)) {
        param->n_keep = json_object_get_int(obj);
    }
    
    // top_p
    if (json_object_object_get_ex(json_param, "top_p", &obj)) {
        param->top_p = (float)json_object_get_double(obj);
    }
    
    // temperature
    if (json_object_object_get_ex(json_param, "temperature", &obj)) {
        param->temperature = (float)json_object_get_double(obj);
    }
    
    // repeat_penalty
    if (json_object_object_get_ex(json_param, "repeat_penalty", &obj)) {
        param->repeat_penalty = (float)json_object_get_double(obj);
    }
    
    // frequency_penalty
    if (json_object_object_get_ex(json_param, "frequency_penalty", &obj)) {
        param->frequency_penalty = (float)json_object_get_double(obj);
    }
    
    // presence_penalty
    if (json_object_object_get_ex(json_param, "presence_penalty", &obj)) {
        param->presence_penalty = (float)json_object_get_double(obj);
    }
    
    // mirostat
    if (json_object_object_get_ex(json_param, "mirostat", &obj)) {
        param->mirostat = json_object_get_int(obj);
    }
    
    // mirostat_tau
    if (json_object_object_get_ex(json_param, "mirostat_tau", &obj)) {
        param->mirostat_tau = (float)json_object_get_double(obj);
    }
    
    // mirostat_eta
    if (json_object_object_get_ex(json_param, "mirostat_eta", &obj)) {
        param->mirostat_eta = (float)json_object_get_double(obj);
    }
    
    // skip_special_token
    if (json_object_object_get_ex(json_param, "skip_special_token", &obj)) {
        param->skip_special_token = json_object_get_boolean(obj);
    }
    
    // is_async
    if (json_object_object_get_ex(json_param, "is_async", &obj)) {
        param->is_async = json_object_get_boolean(obj);
    }
    
    // img_start
    if (json_object_object_get_ex(json_param, "img_start", &obj)) {
        const char* img_start = json_object_get_string(obj);
        if (img_start) {
            param->img_start = strdup(img_start);
            if (!param->img_start) {
                if (param->model_path) free((void*)param->model_path);
                return -1; // Memory allocation failed
            }
        }
    }
    
    // img_end
    if (json_object_object_get_ex(json_param, "img_end", &obj)) {
        const char* img_end = json_object_get_string(obj);
        if (img_end) {
            param->img_end = strdup(img_end);
            if (!param->img_end) {
                if (param->model_path) free((void*)param->model_path);
                if (param->img_start) free((void*)param->img_start);
                return -1; // Memory allocation failed
            }
        }
    }
    
    // img_content
    if (json_object_object_get_ex(json_param, "img_content", &obj)) {
        const char* img_content = json_object_get_string(obj);
        if (img_content) {
            param->img_content = strdup(img_content);
            if (!param->img_content) {
                if (param->model_path) free((void*)param->model_path);
                if (param->img_start) free((void*)param->img_start);
                if (param->img_end) free((void*)param->img_end);
                return -1; // Memory allocation failed
            }
        }
    }
    
    // extend_param (nested object)
    if (json_object_object_get_ex(json_param, "extend_param", &obj)) {
        json_object* extend_obj;
        
        if (json_object_object_get_ex(obj, "base_domain_id", &extend_obj)) {
            param->extend_param.base_domain_id = json_object_get_int(extend_obj);
        }
        
        if (json_object_object_get_ex(obj, "embed_flash", &extend_obj)) {
            param->extend_param.embed_flash = (int8_t)json_object_get_int(extend_obj);
        }
        
        if (json_object_object_get_ex(obj, "enabled_cpus_num", &extend_obj)) {
            param->extend_param.enabled_cpus_num = (int8_t)json_object_get_int(extend_obj);
        }
        
        if (json_object_object_get_ex(obj, "enabled_cpus_mask", &extend_obj)) {
            param->extend_param.enabled_cpus_mask = (uint32_t)json_object_get_int64(extend_obj);
        }
        
        if (json_object_object_get_ex(obj, "n_batch", &extend_obj)) {
            param->extend_param.n_batch = (uint8_t)json_object_get_int(extend_obj);
        }
        
        if (json_object_object_get_ex(obj, "use_cross_attn", &extend_obj)) {
            param->extend_param.use_cross_attn = (int8_t)json_object_get_int(extend_obj);
        }
    }
    
    return 0;
}