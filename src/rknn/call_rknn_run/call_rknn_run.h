#ifndef CALL_RKNN_RUN_H
#define CALL_RKNN_RUN_H

#include <json-c/json.h>

/**
 * Calls rknn_run to execute inference on the vision model
 * @param params JSON parameters containing input tensor data
 * @return JSON response object with inference results
 */
json_object* call_rknn_run(json_object* params);

#endif