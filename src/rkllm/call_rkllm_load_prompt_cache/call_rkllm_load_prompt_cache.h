#ifndef CALL_RKLLM_LOAD_PROMPT_CACHE_H
#define CALL_RKLLM_LOAD_PROMPT_CACHE_H

#include <json-c/json.h>

/**
 * Loads a prompt cache from a file
 * Maps to: int rkllm_load_prompt_cache(LLMHandle handle, const char* prompt_cache_path)
 * @param params JSON array containing prompt cache path
 * @return JSON object with success/error status
 */
json_object* call_rkllm_load_prompt_cache(json_object* params);

#endif