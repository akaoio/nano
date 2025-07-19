#include "core/io/io.h"
#include "mapping/rkllm_proxy/rkllm_proxy.h"
#include "mapping/rkllm_proxy/rkllm_operations.h"
#include <json-c/json.h>
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
    
    // Parse JSON-RPC request using json-c
    // Expected format: {"jsonrpc":"2.0","id":1,"method":"init","params":{...}}
    
    json_object *root = json_tokener_parse(json_request);
    if (!root) {
        return -1;
    }
    
    // Extract ID
    json_object *id_obj;
    if (json_object_object_get_ex(root, "id", &id_obj)) {
        *request_id = (uint32_t)json_object_get_int(id_obj);
    }
    
    // Extract method
    json_object *method_obj;
    if (json_object_object_get_ex(root, "method", &method_obj)) {
        const char *method_str = json_object_get_string(method_obj);
        if (method_str) {
            size_t len = strlen(method_str);
            if (len >= 64) len = 63;  // Ensure we don't overflow 64-byte buffer
            strncpy(method, method_str, len);
            method[len] = '\0';
        } else {
            json_object_put(root);
            return -1;
        }
    } else {
        json_object_put(root);
        return -1;
    }
    
    // Extract params object
    json_object *params_obj;
    if (json_object_object_get_ex(root, "params", &params_obj)) {
        const char *params_str = json_object_to_json_string(params_obj);
        if (params_str) {
            size_t len = strlen(params_str);
            if (len >= 256) len = 255;  // Ensure we don't overflow 256-byte buffer
            strncpy(params, params_str, len);
            params[len] = '\0';
            
            // Extract handle_id from params if present
            json_object *handle_id_obj;
            if (json_object_object_get_ex(params_obj, "handle_id", &handle_id_obj)) {
                *handle_id = (uint32_t)json_object_get_int(handle_id_obj);
            }
        }
    }
    
    json_object_put(root);
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
        // Create error response using json-c
        json_object *response = json_object_new_object();
        json_object *jsonrpc = json_object_new_string("2.0");
        json_object *id = json_object_new_int(request_id);
        json_object *error = json_object_new_object();
        json_object *code = json_object_new_int(-32700);
        json_object *message = json_object_new_string("Parse error");
        
        json_object_object_add(error, "code", code);
        json_object_object_add(error, "message", message);
        json_object_object_add(response, "jsonrpc", jsonrpc);
        json_object_object_add(response, "id", id);
        json_object_object_add(response, "error", error);
        
        const char *response_str = json_object_to_json_string(response);
        strncpy(json_response, response_str, max_response_len - 1);
        json_response[max_response_len - 1] = '\0';
        
        json_object_put(response);
        return -1;
    }
    
    // Get operation from method name
    rkllm_operation_t operation = rkllm_proxy_get_operation_by_name(method);
    if (operation == OP_MAX) {
        // Create method not found error using json-c
        json_object *response = json_object_new_object();
        json_object *jsonrpc = json_object_new_string("2.0");
        json_object *id = json_object_new_int(request_id);
        json_object *error = json_object_new_object();
        json_object *code = json_object_new_int(-32601);
        json_object *message = json_object_new_string("Method not found");
        
        json_object_object_add(error, "code", code);
        json_object_object_add(error, "message", message);
        json_object_object_add(response, "jsonrpc", jsonrpc);
        json_object_object_add(response, "id", id);
        json_object_object_add(response, "error", error);
        
        const char *response_str = json_object_to_json_string(response);
        strncpy(json_response, response_str, max_response_len - 1);
        json_response[max_response_len - 1] = '\0';
        
        json_object_put(response);
        return -1;
    }
    
    // Check for streaming flag in params
    bool streaming_enabled = false;
    json_object *params_obj = json_tokener_parse(params);
    if (params_obj) {
        json_object *stream_obj;
        if (json_object_object_get_ex(params_obj, "stream", &stream_obj)) {
            streaming_enabled = json_object_get_boolean(stream_obj);
        }
        json_object_put(params_obj);
    }
    
    // Create RKLLM request
    rkllm_request_t rkllm_request = {
        .operation = operation,
        .handle_id = handle_id,
        .params_json = params,
        .params_size = strlen(params)
    };
    
    // Execute operation with streaming support
    rkllm_result_t rkllm_result = {0};
    int status;
    
    if (streaming_enabled) {
        // Enable streaming mode - this will need implementation in rkllm_proxy_execute
        status = rkllm_proxy_execute_streaming(&rkllm_request, &rkllm_result, request_id);
    } else {
        // Normal execution
        status = rkllm_proxy_execute(&rkllm_request, &rkllm_result);
    }
    
    // Create JSON-RPC response using json-c
    json_object *response = json_object_new_object();
    json_object *jsonrpc = json_object_new_string("2.0");
    json_object *id = json_object_new_int(request_id);
    
    json_object_object_add(response, "jsonrpc", jsonrpc);
    json_object_object_add(response, "id", id);
    
    if (status == 0) {
        // Success response
        json_object *result;
        if (rkllm_result.result_data) {
            // Try to parse result_data as JSON, if it fails, treat as string
            result = json_tokener_parse(rkllm_result.result_data);
            if (!result) {
                result = json_object_new_string(rkllm_result.result_data);
            }
        } else {
            result = json_object_new_null();
        }
        json_object_object_add(response, "result", result);
    } else {
        // Error response
        json_object *error = json_object_new_object();
        json_object *code = json_object_new_int(status);
        json_object *message = json_object_new_string("Operation failed");
        
        json_object_object_add(error, "code", code);
        json_object_object_add(error, "message", message);
        
        if (rkllm_result.result_data) {
            json_object *data = json_tokener_parse(rkllm_result.result_data);
            if (!data) {
                data = json_object_new_string(rkllm_result.result_data);
            }
            json_object_object_add(error, "data", data);
        }
        
        json_object_object_add(response, "error", error);
    }
    
    const char *response_str = json_object_to_json_string(response);
    strncpy(json_response, response_str, max_response_len - 1);
    json_response[max_response_len - 1] = '\0';
    
    json_object_put(response);
    
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
