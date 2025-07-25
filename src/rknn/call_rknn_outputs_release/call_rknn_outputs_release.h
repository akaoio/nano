#ifndef CALL_RKNN_OUTPUTS_RELEASE_H
#define CALL_RKNN_OUTPUTS_RELEASE_H

#include <json-c/json.h>

/**
 * Release RKNN output tensors
 * @param params JSON object containing output release parameters
 * @return JSON response object (success/error)
 */
json_object* call_rknn_outputs_release(json_object* params);

#endif