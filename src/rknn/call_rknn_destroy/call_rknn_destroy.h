#ifndef CALL_RKNN_DESTROY_H
#define CALL_RKNN_DESTROY_H

#include <json-c/json.h>

/**
 * Calls rknn_destroy to cleanup the vision model
 * @param params JSON parameters (not used)
 * @return JSON response object (success/error)
 */
json_object* call_rknn_destroy(json_object* params);

#endif