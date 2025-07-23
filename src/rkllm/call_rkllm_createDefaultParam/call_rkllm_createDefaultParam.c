#include "call_rkllm_createDefaultParam.h"
#include <stdbool.h>
#include <rkllm.h>

json_object* call_rkllm_createDefaultParam(void) {
    RKLLMParam default_param = rkllm_createDefaultParam();
    
    // Convert RKLLMParam to JSON - exact 1:1 mapping
    json_object* result = json_object_new_object();
    if (!result) {
        return NULL;
    }
    
    // Basic parameters
    json_object_object_add(result, "model_path", 
        default_param.model_path ? json_object_new_string(default_param.model_path) : NULL);
    json_object_object_add(result, "max_context_len", json_object_new_int(default_param.max_context_len));
    json_object_object_add(result, "max_new_tokens", json_object_new_int(default_param.max_new_tokens));
    json_object_object_add(result, "top_k", json_object_new_int(default_param.top_k));
    json_object_object_add(result, "n_keep", json_object_new_int(default_param.n_keep));
    json_object_object_add(result, "top_p", json_object_new_double(default_param.top_p));
    json_object_object_add(result, "temperature", json_object_new_double(default_param.temperature));
    json_object_object_add(result, "repeat_penalty", json_object_new_double(default_param.repeat_penalty));
    json_object_object_add(result, "frequency_penalty", json_object_new_double(default_param.frequency_penalty));
    json_object_object_add(result, "presence_penalty", json_object_new_double(default_param.presence_penalty));
    json_object_object_add(result, "mirostat", json_object_new_int(default_param.mirostat));
    json_object_object_add(result, "mirostat_tau", json_object_new_double(default_param.mirostat_tau));
    json_object_object_add(result, "mirostat_eta", json_object_new_double(default_param.mirostat_eta));
    json_object_object_add(result, "skip_special_token", json_object_new_boolean(default_param.skip_special_token));
    json_object_object_add(result, "is_async", json_object_new_boolean(default_param.is_async));
    
    // Image parameters
    json_object_object_add(result, "img_start", 
        default_param.img_start ? json_object_new_string(default_param.img_start) : NULL);
    json_object_object_add(result, "img_end", 
        default_param.img_end ? json_object_new_string(default_param.img_end) : NULL);
    json_object_object_add(result, "img_content", 
        default_param.img_content ? json_object_new_string(default_param.img_content) : NULL);
    
    // Extended parameters
    json_object* extend_param = json_object_new_object();
    json_object_object_add(extend_param, "base_domain_id", json_object_new_int(default_param.extend_param.base_domain_id));
    json_object_object_add(extend_param, "embed_flash", json_object_new_int(default_param.extend_param.embed_flash));
    json_object_object_add(extend_param, "enabled_cpus_num", json_object_new_int(default_param.extend_param.enabled_cpus_num));
    json_object_object_add(extend_param, "enabled_cpus_mask", json_object_new_int64(default_param.extend_param.enabled_cpus_mask));
    json_object_object_add(extend_param, "n_batch", json_object_new_int(default_param.extend_param.n_batch));
    json_object_object_add(extend_param, "use_cross_attn", json_object_new_int(default_param.extend_param.use_cross_attn));
    json_object_object_add(extend_param, "reserved", NULL); // Reserved array not exposed
    
    json_object_object_add(result, "extend_param", extend_param);
    
    return result;
}