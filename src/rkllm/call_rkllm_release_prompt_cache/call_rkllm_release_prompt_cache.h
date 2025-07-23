#ifndef CALL_RKLLM_RELEASE_PROMPT_CACHE_H
#define CALL_RKLLM_RELEASE_PROMPT_CACHE_H

#include <json-c/json.h>

/**
 * Releases the prompt cache from memory
 * Maps to: int rkllm_release_prompt_cache(LLMHandle handle)
 * @return JSON object with success/error status
 */
json_object* call_rkllm_release_prompt_cache(void);

#endif