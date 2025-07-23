#ifndef CALL_RKLLM_RUN_H
#define CALL_RKLLM_RUN_H

#include <json-c/json.h>

/**
 * Calls rkllm_run with JSON-RPC parameters for synchronous inference with streaming
 * @param params JSON array containing RKLLMInput and RKLLMInferParam
 * @param client_fd Client file descriptor for streaming responses
 * @param request_id Request ID for callback correlation
 * @return JSON object with result or NULL on error
 */
json_object* call_rkllm_run(json_object* params, int client_fd, int request_id);

#endif