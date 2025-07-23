#ifndef CALL_RKLLM_INIT_H
#define CALL_RKLLM_INIT_H

#include <json-c/json.h>
#include <stdbool.h>
#include <rkllm.h>

/**
 * Global LLMHandle - only ONE model can be loaded at a time
 */
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

/**
 * Calls rkllm_init with parameters from JSON-RPC request
 * @param params JSON array containing RKLLMParam object
 * @return JSON response object (success/error)
 */
json_object* call_rkllm_init(json_object* params);

/**
 * Global RKLLM callback function for streaming
 * @param result RKLLM result data
 * @param userdata User context data
 * @param state Callback state
 * @return 0 on success
 */
int global_rkllm_callback(RKLLMResult* result, void* userdata, LLMCallState state);

#endif