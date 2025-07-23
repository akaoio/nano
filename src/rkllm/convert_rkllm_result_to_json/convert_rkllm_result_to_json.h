#ifndef CONVERT_RKLLM_RESULT_TO_JSON_H
#define CONVERT_RKLLM_RESULT_TO_JSON_H

#include <json-c/json.h>
#include <stdbool.h>
#include <rkllm.h>

/**
 * Converts RKLLMResult structure to JSON object with 1:1 mapping
 * @param result RKLLMResult structure from RKLLM callback
 * @param state LLMCallState from RKLLM callback
 * @return JSON object with exact RKLLM structure mapping
 */
json_object* convert_rkllm_result_to_json(RKLLMResult* result, LLMCallState state);

#endif