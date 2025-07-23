#ifndef CALL_RKLLM_SET_CHAT_TEMPLATE_H
#define CALL_RKLLM_SET_CHAT_TEMPLATE_H

#include <json-c/json.h>

/**
 * Sets the chat template for the LLM, including system prompt, prefix, and postfix
 * Maps to: int rkllm_set_chat_template(LLMHandle handle, const char* system_prompt, const char* prompt_prefix, const char* prompt_postfix)
 * @param params JSON array containing chat template parameters
 * @return JSON object with success/error status
 */
json_object* call_rkllm_set_chat_template(json_object* params);

#endif