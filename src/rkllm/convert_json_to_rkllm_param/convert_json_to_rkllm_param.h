#ifndef CONVERT_JSON_TO_RKLLM_PARAM_H
#define CONVERT_JSON_TO_RKLLM_PARAM_H

#include <json-c/json.h>
#include <stdbool.h>
#include <rkllm.h>

/**
 * Converts JSON object to RKLLMParam structure
 * @param json_param JSON object containing RKLLM parameters
 * @param param Pointer to RKLLMParam structure to fill
 * @return 0 on success, -1 on error
 */
int convert_json_to_rkllm_param(json_object* json_param, RKLLMParam* param);

#endif