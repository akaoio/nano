#ifndef CALL_RKLLM_DESTROY_H
#define CALL_RKLLM_DESTROY_H

#include <json-c/json.h>

/**
 * Destroys the RKLLM instance and releases resources
 * Maps to: int rkllm_destroy(LLMHandle handle)
 * @return JSON object with success/error status
 */
json_object* call_rkllm_destroy(void);

#endif