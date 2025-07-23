#ifndef CALL_RKLLM_SET_CROSS_ATTN_PARAMS_H
#define CALL_RKLLM_SET_CROSS_ATTN_PARAMS_H

#include <json-c/json.h>

/**
 * Sets the cross-attention parameters for the LLM decoder
 * Maps to: int rkllm_set_cross_attn_params(LLMHandle handle, RKLLMCrossAttnParam* cross_attn_params)
 * @param params JSON array containing cross-attention parameters
 * @return JSON object with success/error status
 */
json_object* call_rkllm_set_cross_attn_params(json_object* params);

#endif