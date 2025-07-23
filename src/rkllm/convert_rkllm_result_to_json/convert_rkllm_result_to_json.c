#include "convert_rkllm_result_to_json.h"
#include <string.h>
#include <stdlib.h>

json_object* convert_rkllm_result_to_json(RKLLMResult* result, LLMCallState state) {
    if (!result) {
        return NULL;
    }
    
    // Create main result object
    json_object* result_obj = json_object_new_object();
    
    // Add text field (const char* text)
    if (result->text) {
        json_object_object_add(result_obj, "text", json_object_new_string(result->text));
    } else {
        json_object_object_add(result_obj, "text", NULL);
    }
    
    // Add token_id field (int32_t token_id)
    json_object_object_add(result_obj, "token_id", json_object_new_int(result->token_id));
    
    // Add last_hidden_layer field (RKLLMResultLastHiddenLayer)
    json_object* hidden_layer_obj = json_object_new_object();
    
    // Convert hidden_states float array to JSON array
    json_object* hidden_states_array = NULL;
    if (result->last_hidden_layer.hidden_states && result->last_hidden_layer.embd_size > 0) {
        hidden_states_array = json_object_new_array();
        for (int i = 0; i < result->last_hidden_layer.embd_size; i++) {
            json_object* float_val = json_object_new_double(result->last_hidden_layer.hidden_states[i]);
            json_object_array_add(hidden_states_array, float_val);
        }
    }
    json_object_object_add(hidden_layer_obj, "hidden_states", hidden_states_array);
    
    // Add embd_size (int embd_size)
    json_object_object_add(hidden_layer_obj, "embd_size", json_object_new_int(result->last_hidden_layer.embd_size));
    
    // Add num_tokens (int num_tokens)
    json_object_object_add(hidden_layer_obj, "num_tokens", json_object_new_int(result->last_hidden_layer.num_tokens));
    
    json_object_object_add(result_obj, "last_hidden_layer", hidden_layer_obj);
    
    // Add logits field (RKLLMResultLogits)
    json_object* logits_obj = json_object_new_object();
    
    // Convert logits float array to JSON array
    json_object* logits_array = NULL;
    if (result->logits.logits && result->logits.vocab_size > 0) {
        logits_array = json_object_new_array();
        for (int i = 0; i < result->logits.vocab_size; i++) {
            json_object* float_val = json_object_new_double(result->logits.logits[i]);
            json_object_array_add(logits_array, float_val);
        }
    }
    json_object_object_add(logits_obj, "logits", logits_array);
    
    // Add vocab_size (int vocab_size)
    json_object_object_add(logits_obj, "vocab_size", json_object_new_int(result->logits.vocab_size));
    
    // Add num_tokens (int num_tokens)
    json_object_object_add(logits_obj, "num_tokens", json_object_new_int(result->logits.num_tokens));
    
    json_object_object_add(result_obj, "logits", logits_obj);
    
    // Add perf field (RKLLMPerfStat)
    json_object* perf_obj = json_object_new_object();
    
    // Add prefill_time_ms (float)
    json_object_object_add(perf_obj, "prefill_time_ms", json_object_new_double(result->perf.prefill_time_ms));
    
    // Add prefill_tokens (int)
    json_object_object_add(perf_obj, "prefill_tokens", json_object_new_int(result->perf.prefill_tokens));
    
    // Add generate_time_ms (float)
    json_object_object_add(perf_obj, "generate_time_ms", json_object_new_double(result->perf.generate_time_ms));
    
    // Add generate_tokens (int)
    json_object_object_add(perf_obj, "generate_tokens", json_object_new_int(result->perf.generate_tokens));
    
    // Add memory_usage_mb (float)
    json_object_object_add(perf_obj, "memory_usage_mb", json_object_new_double(result->perf.memory_usage_mb));
    
    json_object_object_add(result_obj, "perf", perf_obj);
    
    // Add callback state (LLMCallState state)
    json_object_object_add(result_obj, "_callback_state", json_object_new_int(state));
    
    return result_obj;
}