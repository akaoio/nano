#ifndef CALL_RKLLM_LOAD_LORA_H
#define CALL_RKLLM_LOAD_LORA_H

#include <json-c/json.h>

/**
 * Loads a LoRA adapter into the LLM
 * Maps to: int rkllm_load_lora(LLMHandle handle, RKLLMLoraAdapter* lora_adapter)
 * @param params JSON array containing RKLLMLoraAdapter parameters
 * @return JSON object with success/error status
 */
json_object* call_rkllm_load_lora(json_object* params);

#endif