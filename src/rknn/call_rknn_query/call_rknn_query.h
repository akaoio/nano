#ifndef CALL_RKNN_QUERY_H
#define CALL_RKNN_QUERY_H

#include <json-c/json.h>
#include <rknn_api.h>

/**
 * Calls rknn_query to get model information
 * @param params JSON array containing query type
 * @return JSON response object with model information
 */
json_object* call_rknn_query(json_object* params);

#endif