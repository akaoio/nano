#ifndef CALL_RKLLM_CLEAR_KV_CACHE_H
#define CALL_RKLLM_CLEAR_KV_CACHE_H

#include <json-c/json.h>

/**
 * Clears the key-value cache for a given LLM handle
 * Maps to: int rkllm_clear_kv_cache(LLMHandle handle, int keep_system_prompt, int* start_pos, int* end_pos)
 * @param params JSON array containing cache clearing parameters
 * @return JSON object with success/error status
 */
json_object* call_rkllm_clear_kv_cache(json_object* params);

#endif