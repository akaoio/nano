#ifndef CALL_RKLLM_GET_KV_CACHE_SIZE_H
#define CALL_RKLLM_GET_KV_CACHE_SIZE_H

#include <json-c/json.h>

/**
 * Gets the current size of the key-value cache for a given LLM handle
 * Maps to: int rkllm_get_kv_cache_size(LLMHandle handle, int* cache_sizes)
 * @return JSON object with cache sizes or error status
 */
json_object* call_rkllm_get_kv_cache_size(void);

#endif