#pragma once

#include "rkllm_proxy.h"
#include <json-c/json.h>

// RKLLM Operations - Individual operation handlers
// Each operation has a handler function that:
// 1. Parses JSON parameters
// 2. Calls corresponding RKLLM function
// 3. Formats result as JSON

/**
 * @brief Handler for init operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_init(uint32_t* handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for destroy operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_destroy(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for run operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_run(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for run_async operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_run_async(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for abort operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_abort(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for is_running operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_is_running(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for load_lora operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_load_lora(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for load_prompt_cache operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_load_prompt_cache(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for release_prompt_cache operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_release_prompt_cache(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for clear_kv_cache operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_clear_kv_cache(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for get_kv_cache_size operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_get_kv_cache_size(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for set_chat_template operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_set_chat_template(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for set_function_tools operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_set_function_tools(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for set_cross_attn_params operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_set_cross_attn_params(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

/**
 * @brief Handler for create_default_param operation
 * @param handle_id Handle ID for the operation
 * @param params_json JSON parameters
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_op_create_default_param(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

// Operation handler function pointer type
typedef int (*rkllm_op_handler_t)(uint32_t handle_id, const char* params_json, rkllm_result_t* result);

// Operation handler for init (special case - creates handle)
typedef int (*rkllm_op_init_handler_t)(uint32_t* handle_id, const char* params_json, rkllm_result_t* result);

// Operation handler table
extern const rkllm_op_handler_t OPERATION_HANDLERS[OP_MAX];
extern const rkllm_op_init_handler_t INIT_HANDLER;
