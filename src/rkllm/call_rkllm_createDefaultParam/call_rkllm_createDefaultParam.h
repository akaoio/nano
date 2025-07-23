#ifndef CALL_RKLLM_CREATE_DEFAULT_PARAM_H
#define CALL_RKLLM_CREATE_DEFAULT_PARAM_H

#include <json-c/json.h>

/**
 * Calls rkllm_createDefaultParam and returns JSON result
 * @return JSON object with default parameters or NULL on error
 */
json_object* call_rkllm_createDefaultParam(void);

#endif