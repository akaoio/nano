#include "core/io/io.h"
#include "mapping/rkllm_proxy/rkllm_proxy.h"
#include "mapping/rkllm_proxy/rkllm_operations.h"
#include "common/json_utils/json_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// IO Operations - Bridge between IO core and RKLLM proxy
// Handles JSON request parsing and routing to appropriate RKLLM operations

/**
 * @brief Parse JSON-RPC request and extract operation details
 * @param json_request JSON-RPC request string
 * @param request_id Output: Request ID from JSON-RPC
 * @param handle_id Output: Handle ID from params
 * @param method Output: Method name
 * @param params Output: Parameters JSON string
 * @return 0 on success, -1 on error
 */
int io_parse_json_request(const char* json_request, uint32_t* request_id, 
                         uint32_t* handle_id, char* method, char* params) {
    if (!json_request || !request_id || !handle_id || !method || !params) {
        return -1;
    }
    
    // Initialize outputs
    *request_id = 0;
    *handle_id = 0;
    method[0] = '\0';
    params[0] = '\0';
    
    // Parse JSON-RPC request
    // Expected format: {"jsonrpc":"2.0","id":1,"method":"init","params":{...}}
    
    // Extract ID
    const char* id_start = strstr(json_request, "\"id\":");
    if (id_start) {
        *request_id = (uint32_t)strtoul(id_start + 5, NULL, 10);
    }
    
    // Extract method
    const char* method_start = strstr(json_request, "\"method\":");
    if (method_start) {
        method_start = strchr(method_start + 8, '"'); // Skip "method": part
        if (method_start) {
            method_start++; // Skip opening quote
            const char* method_end = strchr(method_start, '"');
            if (method_end) {
                size_t method_len = method_end - method_start;
                if (method_len < 256) { // Assuming 256 is max method length
                    strncpy(method, method_start, method_len);
                    method[method_len] = '\0';
                }
            }
        }
    }
    
    // Extract params
    const char* params_start = strstr(json_request, "\"params\":");
    if (params_start) {
        params_start = strchr(params_start, '{');
        if (params_start) {
            // Find matching closing brace
            int brace_count = 1;
            const char* params_end = params_start + 1;
            while (*params_end && brace_count > 0) {
                if (*params_end == '{') brace_count++;
                else if (*params_end == '}') brace_count--;
                params_end++;
            }
            
            if (brace_count == 0) {
                size_t params_len = params_end - params_start;
                if (params_len < 2048) { // Assuming 2048 is max params length
                    strncpy(params, params_start, params_len);
                    params[params_len] = '\0';
                }
            }
        }
    }
    
    // Extract handle_id from params if present
    if (strlen(params) > 0) {
        const char* handle_start = strstr(params, "\"handle_id\":");
        if (handle_start) {
            *handle_id = (uint32_t)strtoul(handle_start + 12, NULL, 10);
        }
    }
    
    return 0;
}

/**
 * @brief Process IO operation request
 * @param json_request JSON-RPC request
 * @param json_response Output buffer for JSON-RPC response
 * @param max_response_len Maximum response buffer length
 * @return 0 on success, -1 on error
 */
int io_process_request(const char* json_request, char* json_response, size_t max_response_len) {
    if (!json_request || !json_response) {
        return -1;
    }
    
    // Parse request
    uint32_t request_id = 0;
    uint32_t handle_id = 0;
    char method[256] = {0};
    char params[2048] = {0};
    
    if (io_parse_json_request(json_request, &request_id, &handle_id, method, params) != 0) {
        // Create error response
        snprintf(json_response, max_response_len,
                "{\"jsonrpc\":\"2.0\",\"id\":%u,\"error\":{\"code\":-32700,\"message\":\"Parse error\"}}",
                request_id);
        return -1;
    }
    
    // Get operation from method name
    rkllm_operation_t operation = rkllm_proxy_get_operation_by_name(method);
    if (operation == OP_MAX) {
        // Create method not found error
        snprintf(json_response, max_response_len,
                "{\"jsonrpc\":\"2.0\",\"id\":%u,\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}",
                request_id);
        return -1;
    }
    
    // Create RKLLM request
    rkllm_request_t rkllm_request = {
        .operation = operation,
        .handle_id = handle_id,
        .params_json = params,
        .params_size = strlen(params)
    };
    
    // Execute operation
    rkllm_result_t rkllm_result = {0};
    int status = rkllm_proxy_execute(&rkllm_request, &rkllm_result);
    
    // Create JSON-RPC response
    if (status == 0) {
        // Success response
        snprintf(json_response, max_response_len,
                "{\"jsonrpc\":\"2.0\",\"id\":%u,\"result\":%s}",
                request_id, rkllm_result.result_data ? rkllm_result.result_data : "null");
    } else {
        // Error response
        snprintf(json_response, max_response_len,
                "{\"jsonrpc\":\"2.0\",\"id\":%u,\"error\":{\"code\":%d,\"message\":\"Operation failed\",\"data\":%s}}",
                request_id, status, rkllm_result.result_data ? rkllm_result.result_data : "null");
    }
    
    // Cleanup
    rkllm_proxy_free_result(&rkllm_result);
    
    return status;
}

/**
 * @brief Initialize IO operations system
 * @return 0 on success, -1 on error
 */
int io_operations_init(void) {
    return rkllm_proxy_init();
}

/**
 * @brief Shutdown IO operations system
 */
void io_operations_shutdown(void) {
    rkllm_proxy_shutdown();
}
