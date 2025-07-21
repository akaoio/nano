#ifndef RKLLM_ERROR_MAPPING_H
#define RKLLM_ERROR_MAPPING_H

#include <stdint.h>

// RKLLM Error Codes (based on common patterns from Rockchip libraries)
// These are negative values returned by RKLLM functions
typedef enum {
    RKLLM_SUCCESS = 0,
    RKLLM_ERROR_INVALID_PARAM = -1,
    RKLLM_ERROR_MEMORY_ALLOC = -2,
    RKLLM_ERROR_MODEL_LOAD = -3,
    RKLLM_ERROR_INVALID_HANDLE = -4,
    RKLLM_ERROR_NOT_INITIALIZED = -5,
    RKLLM_ERROR_ALREADY_INITIALIZED = -6,
    RKLLM_ERROR_INVALID_MODEL = -7,
    RKLLM_ERROR_INFERENCE_FAILED = -8,
    RKLLM_ERROR_ABORTED = -9,
    RKLLM_ERROR_TIMEOUT = -10,
    RKLLM_ERROR_INVALID_CONFIG = -11,
    RKLLM_ERROR_FILE_NOT_FOUND = -12,
    RKLLM_ERROR_FILE_READ = -13,
    RKLLM_ERROR_UNSUPPORTED = -14,
    RKLLM_ERROR_BUSY = -15,
    RKLLM_ERROR_QUEUE_FULL = -16,
    RKLLM_ERROR_INTERNAL = -99,
    RKLLM_ERROR_UNKNOWN = -100
} rkllm_error_code_t;

// JSON-RPC 2.0 Error Codes (standard + custom range)
typedef enum {
    JSON_RPC_PARSE_ERROR = -32700,
    JSON_RPC_INVALID_REQUEST = -32600,
    JSON_RPC_METHOD_NOT_FOUND = -32601,
    JSON_RPC_INVALID_PARAMS = -32602,
    JSON_RPC_INTERNAL_ERROR = -32603,
    
    // Custom error codes for RKLLM (range -32000 to -32099)
    JSON_RPC_RKLLM_INIT_FAILED = -32001,
    JSON_RPC_RKLLM_INVALID_MODEL = -32002,
    JSON_RPC_RKLLM_MEMORY_ERROR = -32003,
    JSON_RPC_RKLLM_INFERENCE_ERROR = -32004,
    JSON_RPC_RKLLM_TIMEOUT = -32005,
    JSON_RPC_RKLLM_ABORTED = -32006,
    JSON_RPC_RKLLM_BUSY = -32007,
    JSON_RPC_RKLLM_FILE_ERROR = -32008,
    JSON_RPC_RKLLM_CONFIG_ERROR = -32009,
    JSON_RPC_RKLLM_NOT_INITIALIZED = -32010,
    JSON_RPC_RKLLM_UNSUPPORTED = -32011
} json_rpc_error_code_t;

// Error mapping structure
typedef struct {
    int rkllm_error_code;
    int json_rpc_error_code;
    const char* error_message;
    const char* error_data;  // Additional context
} rkllm_error_mapping_t;

// Function to map RKLLM error to JSON-RPC error
int rkllm_map_error_to_json_rpc(int rkllm_error, const char** message, const char** data);

// Function to create JSON error response
char* rkllm_create_error_response(uint32_t request_id, int rkllm_error, const char* method);

// Get human-readable error message for RKLLM error
const char* rkllm_get_error_message(int rkllm_error);

// Helper function to log RKLLM errors with context
void rkllm_log_error(int rkllm_error, const char* function_name, const char* context);

#endif // RKLLM_ERROR_MAPPING_H