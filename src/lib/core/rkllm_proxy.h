#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <json-c/json.h>
#include "../../external/rkllm/rkllm.h"

// Dynamic RKLLM API Proxy - Automatically exposes full RKLLM API
// Provides dynamic function registration and automatic parameter conversion

// Parameter types for RKLLM functions
typedef enum {
    RKLLM_PARAM_HANDLE,           // LLMHandle
    RKLLM_PARAM_HANDLE_PTR,       // LLMHandle*
    RKLLM_PARAM_RKLLM_PARAM_PTR,  // RKLLMParam*
    RKLLM_PARAM_RKLLM_INPUT_PTR,  // RKLLMInput*
    RKLLM_PARAM_RKLLM_INFER_PARAM_PTR, // RKLLMInferParam*
    RKLLM_PARAM_RKLLM_LORA_ADAPTER_PTR, // RKLLMLoraAdapter*
    RKLLM_PARAM_RKLLM_CROSS_ATTN_PARAM_PTR, // RKLLMCrossAttnParam*
    RKLLM_PARAM_RKLLM_LORA_PARAM_PTR, // RKLLMLoraParam*
    RKLLM_PARAM_RKLLM_PROMPT_CACHE_PARAM_PTR, // RKLLMPromptCacheParam*
    RKLLM_PARAM_RKLLM_EXTEND_PARAM_PTR, // RKLLMExtendParam*
    RKLLM_PARAM_RKLLM_RESULT_PTR,              // RKLLMResult*
    RKLLM_PARAM_RKLLM_EMBED_INPUT_PTR,         // RKLLMEmbedInput*  
    RKLLM_PARAM_RKLLM_TOKEN_INPUT_PTR,         // RKLLMTokenInput*
    RKLLM_PARAM_RKLLM_MULTIMODEL_INPUT_PTR,    // RKLLMMultiModelInput*
    RKLLM_PARAM_RKLLM_RESULT_LAST_HIDDEN_LAYER_PTR,  // RKLLMResultLastHiddenLayer*
    RKLLM_PARAM_RKLLM_RESULT_LOGITS_PTR,       // RKLLMResultLogits*
    RKLLM_PARAM_RKLLM_PERF_STAT_PTR,           // RKLLMPerfStat*
    RKLLM_PARAM_CALLBACK,         // LLMResultCallback
    RKLLM_PARAM_STRING,           // const char*
    RKLLM_PARAM_INT,              // int
    RKLLM_PARAM_INT_PTR,          // int*
    RKLLM_PARAM_VOID_PTR,         // void*
    RKLLM_PARAM_BOOL,             // bool
    RKLLM_PARAM_FLOAT,            // float
    RKLLM_PARAM_FLOAT_PTR,        // float*
    RKLLM_PARAM_SIZE_T,           // size_t
    RKLLM_PARAM_INT32_T,          // int32_t
    RKLLM_PARAM_INT32_T_PTR,      // int32_t*
    RKLLM_PARAM_UINT32_T,         // uint32_t
    RKLLM_PARAM_UINT8_T           // uint8_t
} rkllm_param_type_t;

// Return types for RKLLM functions
typedef enum {
    RKLLM_RETURN_INT,             // int (status code)
    RKLLM_RETURN_VOID,            // void
    RKLLM_RETURN_RKLLM_PARAM,     // RKLLMParam (for createDefaultParam)
    RKLLM_RETURN_JSON             // JSON string (for constants)
} rkllm_return_type_t;

// Function parameter descriptor
typedef struct {
    rkllm_param_type_t type;
    const char* name;
    bool is_output;  // true if parameter is output (e.g., handle*)
} rkllm_param_desc_t;

// Function descriptor for dynamic dispatch
typedef struct {
    const char* name;
    void* function_ptr;
    rkllm_return_type_t return_type;
    int param_count;
    rkllm_param_desc_t params[8];  // Max 8 parameters
    const char* description;
} rkllm_function_desc_t;

/**
 * @brief Initialize the dynamic RKLLM proxy system
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_init(void);

/**
 * @brief Shutdown the dynamic RKLLM proxy system
 */
void rkllm_proxy_shutdown(void);

/**
 * @brief Process RKLLM function call dynamically
 * @param function_name RKLLM function name (e.g., "rkllm_init")
 * @param params_json JSON parameters object
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_call(const char* function_name, const char* params_json, char** result_json);

/**
 * @brief Get list of available RKLLM functions
 * @param functions_json Output: JSON array of function descriptions
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_get_functions(char** functions_json);

/**
 * @brief Get all RKLLM constants and enums
 * @param constants_json Output: JSON object of all constants and enums
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_get_constants(char** constants_json);

/**
 * @brief Convert JSON parameter to RKLLM structure
 * @param json_value JSON value
 * @param param_type Parameter type
 * @param output Output buffer for converted parameter
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_convert_param(json_object* json_value, rkllm_param_type_t param_type, 
                              void* output, size_t output_size);

/**
 * @brief Convert RKLLM result to JSON with error mapping
 * @param return_type Return type
 * @param result_data Result data
 * @param result_json Output: JSON result string (caller must free)
 * @param function_name Function name for error context
 * @param status RKLLM function return status
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_convert_result(rkllm_return_type_t return_type, void* result_data, 
                               char** result_json, const char* function_name, int status);

/**
 * @brief Get global RKLLM handle (for functions that need it)
 * @return Current handle or NULL if not initialized
 */
LLMHandle rkllm_proxy_get_handle(void);

/**
 * @brief Set global RKLLM handle (when rkllm_init is called)
 * @param handle Handle to set
 */
void rkllm_proxy_set_handle(LLMHandle handle);

/**
 * @brief Start streaming session for real-time token streaming
 * @param request_id JSON-RPC request ID
 * @param method RKLLM method name (e.g., "rkllm_run_async")
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_start_streaming_session(const char* request_id, const char* method);

/**
 * @brief Stop current streaming session
 */
void rkllm_proxy_stop_streaming_session(void);

/**
 * @brief Check if streaming is currently active
 * @return true if streaming session is active
 */
bool rkllm_proxy_is_streaming_active(void);

/**
 * @brief Get current request ID for active streaming session
 * @return Current request ID or NULL if no active session
 */
const char* rkllm_proxy_get_current_request_id(void);