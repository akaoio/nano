#ifndef ERROR_MAPPING_H
#define ERROR_MAPPING_H

#include <json-c/json.h>

/**
 * @file error_mapping.h
 * @brief RKLLM Error Code to JSON-RPC Error Mapping System
 * 
 * Maps RKLLM library error codes to standardized JSON-RPC 2.0 error responses.
 * Provides consistent error handling across all transport layers.
 */

// RKLLM Error Codes (from RKLLM library)
#define RKLLM_SUCCESS                0
#define RKLLM_INVALID_PARAM         -1
#define RKLLM_MODEL_NOT_FOUND       -2
#define RKLLM_MEMORY_ERROR          -3
#define RKLLM_INFERENCE_ERROR       -4
#define RKLLM_DEVICE_ERROR          -5
#define RKLLM_TIMEOUT_ERROR         -6
#define RKLLM_CONTEXT_ERROR         -7
#define RKLLM_TOKEN_ERROR           -8
#define RKLLM_CALLBACK_ERROR        -9
#define RKLLM_FILE_ERROR           -10
#define RKLLM_NETWORK_ERROR        -11
#define RKLLM_PERMISSION_ERROR     -12
#define RKLLM_VERSION_ERROR        -13
#define RKLLM_INIT_ERROR          -14
#define RKLLM_RESOURCE_BUSY       -15

// JSON-RPC 2.0 Standard Error Codes
#define JSON_RPC_PARSE_ERROR      -32700
#define JSON_RPC_INVALID_REQUEST  -32600
#define JSON_RPC_METHOD_NOT_FOUND -32601
#define JSON_RPC_INVALID_PARAMS   -32602
#define JSON_RPC_INTERNAL_ERROR   -32603

typedef struct {
    int rkllm_code;
    int json_rpc_code;
    const char* message;
    const char* description;
} error_mapping_t;

/**
 * @brief Initialize the error mapping system
 * @return 0 on success, -1 on failure
 */
int error_mapping_init(void);

/**
 * @brief Shutdown the error mapping system
 */
void error_mapping_shutdown(void);

/**
 * @brief Check if error mapping system is initialized
 * @return true if initialized, false otherwise
 */
bool error_mapping_is_initialized(void);

/**
 * @brief Map RKLLM error code to JSON-RPC error code and message
 * @param rkllm_code RKLLM error code
 * @param json_rpc_code Output: JSON-RPC error code
 * @param message Output: Error message
 * @param description Output: Detailed error description
 * @return 0 if mapping found, -1 if using default mapping
 */
int map_rkllm_error_to_json_rpc(int rkllm_code, int* json_rpc_code, 
                                const char** message, const char** description);

/**
 * @brief Create JSON-RPC error response from RKLLM error code
 * @param rkllm_code RKLLM error code
 * @param request_id Request ID (can be NULL)
 * @param additional_data Additional error data (can be NULL)
 * @return JSON object containing error response (caller must free)
 */
json_object* create_error_response_from_rkllm(int rkllm_code, const char* request_id, 
                                             json_object* additional_data);

/**
 * @brief Create standard JSON-RPC error response
 * @param json_rpc_code JSON-RPC error code
 * @param message Error message
 * @param request_id Request ID (can be NULL)
 * @param additional_data Additional error data (can be NULL)
 * @return JSON object containing error response (caller must free)
 */
json_object* create_json_rpc_error_response(int json_rpc_code, const char* message, 
                                           const char* request_id, json_object* additional_data);

/**
 * @brief Get human-readable error description for RKLLM code
 * @param rkllm_code RKLLM error code
 * @return Error description string
 */
const char* get_rkllm_error_description(int rkllm_code);

/**
 * @brief Check if RKLLM error code is recoverable
 * @param rkllm_code RKLLM error code
 * @return true if error is recoverable, false otherwise
 */
bool is_rkllm_error_recoverable(int rkllm_code);

/**
 * @brief Log error with context information
 * @param rkllm_code RKLLM error code
 * @param context Additional context string
 * @param function Function where error occurred
 * @param file Source file name
 * @param line Source line number
 */
void log_rkllm_error(int rkllm_code, const char* context, const char* function, 
                     const char* file, int line);

// Convenience macro for error logging
#define LOG_RKLLM_ERROR(code, context) \
    log_rkllm_error(code, context, __FUNCTION__, __FILE__, __LINE__)

#endif // ERROR_MAPPING_H