#ifndef CONVERT_JSON_TO_RKLLM_INPUT_H
#define CONVERT_JSON_TO_RKLLM_INPUT_H

#include <json-c/json.h>
#include <stdbool.h>
#include <rkllm.h>

/**
 * Converts JSON object to RKLLMInput structure with 1:1 mapping
 * @param json_obj JSON object containing RKLLMInput parameters
 * @param rkllm_input Pointer to RKLLMInput structure to fill
 * @return 0 on success, -1 on error
 */
int convert_json_to_rkllm_input(json_object* json_obj, RKLLMInput* rkllm_input);

#endif