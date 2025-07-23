#ifndef CONVERT_JSON_TO_RKLLM_INFER_PARAM_H
#define CONVERT_JSON_TO_RKLLM_INFER_PARAM_H

#include <json-c/json.h>
#include <stdbool.h>
#include <rkllm.h>

/**
 * Converts JSON object to RKLLMInferParam structure with 1:1 mapping
 * @param json_obj JSON object containing RKLLMInferParam parameters
 * @param rkllm_infer_param Pointer to RKLLMInferParam structure to fill
 * @return 0 on success, -1 on error
 */
int convert_json_to_rkllm_infer_param(json_object* json_obj, RKLLMInferParam* rkllm_infer_param);

#endif