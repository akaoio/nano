#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../../../libs/rkllm/rkllm.h"

// RKLLM Proxy - Maps RKLLM functions without prefix
// Automatically generated mappings for all RKLLM functions

// Operation definitions
typedef enum {
    OP_INIT = 0,
    OP_DESTROY,
    OP_RUN,
    OP_RUN_ASYNC,
    OP_ABORT,
    OP_IS_RUNNING,
    OP_LOAD_LORA,
    OP_LOAD_PROMPT_CACHE,
    OP_RELEASE_PROMPT_CACHE,
    OP_CLEAR_KV_CACHE,
    OP_GET_KV_CACHE_SIZE,
    OP_SET_CHAT_TEMPLATE,
    OP_SET_FUNCTION_TOOLS,
    OP_SET_CROSS_ATTN_PARAMS,
    OP_CREATE_DEFAULT_PARAM,
    OP_MAX
} rkllm_operation_t;

// Operation result structure
typedef struct {
    int status;
    uint32_t handle_id;
    char* result_data;
    size_t result_size;
} rkllm_result_t;

// Forward declaration for streaming callback
typedef void (*rkllm_stream_callback_t)(const char* chunk, bool is_final, void* userdata);

// Callback context for capturing model output
typedef struct {
    char* output_buffer;        // Legacy buffer for non-streaming
    size_t buffer_size;
    size_t current_pos;
    int final_status;
    LLMCallState call_state;
    
    // Streaming support
    uint32_t request_id;        // Request ID for response routing
    bool streaming_enabled;     // Whether to use streaming
    rkllm_stream_callback_t stream_callback;  // Callback for streaming chunks
    void* stream_userdata;      // Data for streaming callback
} rkllm_callback_context_t;

// Operation request structure
typedef struct {
    rkllm_operation_t operation;
    uint32_t handle_id;
    char* params_json;
    size_t params_size;
} rkllm_request_t;

/**
 * @brief Initialize RKLLM proxy
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_init(void);

/**
 * @brief Execute RKLLM operation
 * @param request Operation request
 * @param result Operation result
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_execute(const rkllm_request_t* request, rkllm_result_t* result);

/**
 * @brief Execute RKLLM operation with streaming support
 * @param request Operation request
 * @param result Operation result
 * @param request_id Request ID for streaming responses
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_execute_streaming(const rkllm_request_t* request, rkllm_result_t* result, uint32_t request_id);

/**
 * @brief Shutdown RKLLM proxy
 */
void rkllm_proxy_shutdown(void);

/**
 * @brief Get operation name from enum
 * @param op Operation enum
 * @return Operation name string
 */
const char* rkllm_proxy_get_operation_name(rkllm_operation_t op);

/**
 * @brief Get operation enum from name
 * @param name Operation name
 * @return Operation enum
 */
rkllm_operation_t rkllm_proxy_get_operation_by_name(const char* name);

/**
 * @brief Get handle from handle ID
 * @param handle_id Handle ID
 * @return LLMHandle or nullptr if not found
 */
LLMHandle rkllm_proxy_get_handle(uint32_t handle_id);

/**
 * @brief Create JSON result
 * @param status Status code
 * @param data Data string
 * @return Allocated JSON result string
 */
char* rkllm_proxy_create_json_result(int status, const char* data);

/**
 * @brief Create error result
 * @param status Error status code
 * @param error_msg Error message
 * @return Allocated error result string
 */
char* rkllm_proxy_create_error_result(int status, const char* error_msg);

/**
 * @brief Free result data
 * @param result Result to free
 */
void rkllm_proxy_free_result(rkllm_result_t* result);

/**
 * @brief Global callback function for RKLLM results
 * @param result RKLLM result structure
 * @param userdata User data (callback context)
 * @param state Current call state
 * @return 0 to continue, 1 to pause
 */
int rkllm_proxy_global_callback(RKLLMResult* result, void* userdata, LLMCallState state);

/**
 * @brief Create callback context for capturing output
 * @param buffer_size Size of output buffer
 * @return Allocated context or nullptr on error
 */
rkllm_callback_context_t* rkllm_proxy_create_callback_context(size_t buffer_size);

/**
 * @brief Destroy callback context
 * @param context Context to destroy
 */
void rkllm_proxy_destroy_callback_context(rkllm_callback_context_t* context);

// Operation name mapping
static const char* const OPERATION_NAMES[OP_MAX] = {
    [OP_INIT] = "init",
    [OP_DESTROY] = "destroy",
    [OP_RUN] = "run",
    [OP_RUN_ASYNC] = "run_async",
    [OP_ABORT] = "abort",
    [OP_IS_RUNNING] = "is_running",
    [OP_LOAD_LORA] = "load_lora",
    [OP_LOAD_PROMPT_CACHE] = "load_prompt_cache",
    [OP_RELEASE_PROMPT_CACHE] = "release_prompt_cache",
    [OP_CLEAR_KV_CACHE] = "clear_kv_cache",
    [OP_GET_KV_CACHE_SIZE] = "get_kv_cache_size",
    [OP_SET_CHAT_TEMPLATE] = "set_chat_template",
    [OP_SET_FUNCTION_TOOLS] = "set_function_tools",
    [OP_SET_CROSS_ATTN_PARAMS] = "set_cross_attn_params",
    [OP_CREATE_DEFAULT_PARAM] = "create_default_param"
};
