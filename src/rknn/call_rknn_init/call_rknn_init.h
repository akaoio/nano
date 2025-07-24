#ifndef CALL_RKNN_INIT_H
#define CALL_RKNN_INIT_H

#include <json-c/json.h>
#include <stdbool.h>
#include <rknn_api.h>

/**
 * Global RKNN context - only ONE vision model can be loaded at a time
 */
extern rknn_context global_rknn_context;
extern int global_rknn_initialized;

/**
 * Calls rknn_init with parameters from JSON-RPC request
 * @param params JSON array containing model_path and core_mask
 * @return JSON response object (success/error)
 */
json_object* call_rknn_init(json_object* params);

#endif