#ifndef CALL_RKLLM_SET_FUNCTION_TOOLS_H
#define CALL_RKLLM_SET_FUNCTION_TOOLS_H

#include <json-c/json.h>

/**
 * Sets the function calling configuration for the LLM
 * Maps to: int rkllm_set_function_tools(LLMHandle handle, const char* system_prompt, const char* tools, const char* tool_response_str)
 * @param params JSON array containing function tools configuration
 * @return JSON object with success/error status
 */
json_object* call_rkllm_set_function_tools(json_object* params);

#endif