#ifndef CALL_RKLLM_RUN_ASYNC_H
#define CALL_RKLLM_RUN_ASYNC_H

#include <json-c/json.h>

/**
 * Calls rkllm_run_async with JSON-RPC parameters for async inference
 * @param params JSON array containing RKLLMInput and RKLLMInferParam
 * @param client_fd File descriptor of the client for callback context
 * @param request_id JSON-RPC request ID for response correlation
 * @return JSON object with result or NULL on error
 */
json_object* call_rkllm_run_async(json_object* params, int client_fd, int request_id);

#endif