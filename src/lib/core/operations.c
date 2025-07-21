#include "operations.h"
#include "rkllm_proxy.h"
#include "../../external/rkllm/rkllm.h"
#include "../../common/string_utils/string_utils.h"
#include "../protocol/streaming.h"
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// IO Operations - Direct RKLLM integration (lightweight approach)
// Handles JSON request parsing and direct RKLLM function calls


/**
 * @brief Parse JSON-RPC request and extract operation details (main version)
 * @param json_request JSON-RPC request string
 * @param request_id Output: Request ID from JSON-RPC
 * @param method Output: Method name
 * @param params Output: Parameters JSON string
 * @return 0 on success, -1 on error
 */
int io_parse_json_request_main(const char* json_request, uint32_t* request_id, 
                               char* method, char* params) {
    if (!json_request || !request_id || !method || !params) {
        return -1;
    }
    
    // Initialize outputs
    *request_id = 0;
    method[0] = '\0';
    params[0] = '\0';
    
    json_object* root = json_tokener_parse(json_request);
    if (!root) {
        return -1;
    }
    
    // Extract request ID (optional)
    json_object* id_obj;
    if (json_object_object_get_ex(root, "id", &id_obj)) {
        *request_id = json_object_get_int(id_obj);
    }
    
    // Extract method (required)
    json_object* method_obj;
    if (!json_object_object_get_ex(root, "method", &method_obj)) {
        json_object_put(root);
        return -1;
    }
    strncpy(method, json_object_get_string(method_obj), 255);
    method[255] = '\0';
    
    // Extract params (optional)
    json_object* params_obj;
    if (json_object_object_get_ex(root, "params", &params_obj)) {
        const char* params_str = json_object_to_json_string(params_obj);
        strncpy(params, params_str, 4095);
        params[4095] = '\0';
    }
    
    json_object_put(root);
    return 0;
}

/**
 * @brief Create JSON response
 * @param request_id Request ID
 * @param success Success flag
 * @param data Response data
 * @return Allocated JSON response string (caller must free)
 */
char* io_create_json_response(uint32_t request_id, bool success, const char* data) {
    json_object* response = json_object_new_object();
    
    // Add request ID
    json_object* id_obj = json_object_new_int(request_id);
    json_object_object_add(response, "id", id_obj);
    
    if (success) {
        // Success response
        json_object* result_obj = json_object_new_object();
        if (data) {
            json_object* data_obj = json_object_new_string(data);
            json_object_object_add(result_obj, "data", data_obj);
        }
        json_object_object_add(response, "result", result_obj);
    } else {
        // Error response
        json_object* error_obj = json_object_new_object();
        json_object* code_obj = json_object_new_int(-1);
        json_object* message_obj = json_object_new_string(data ? data : "Unknown error");
        json_object_object_add(error_obj, "code", code_obj);
        json_object_object_add(error_obj, "message", message_obj);
        json_object_object_add(response, "error", error_obj);
    }
    
    const char* json_str = json_object_to_json_string(response);
    char* result = strdup(json_str);
    json_object_put(response);
    
    return result;
}


/**
 * @brief Process operation request using dynamic RKLLM proxy
 * @param method Operation method name
 * @param params_json Parameters JSON string
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_process_operation(const char* method, const char* params_json, char** result_json) {
    printf("[DEBUG] io_process_operation called with method: %s\n", method ? method : "NULL");
    fflush(stdout);
    
    if (!method || !result_json) {
        printf("🔧 DEBUG: Early return from io_process_operation\n");
        fflush(stdout);
        return -1;
    }
    
    // Special handling for streaming requests
    if (strcmp(method, "rkllm_run_streaming") == 0) {
        // Streaming operations are handled by the test framework
        // Return success to indicate the operation was recognized
        *result_json = strdup("{\"status\": \"streaming_initiated\", \"note\": \"Handled by callback mechanism\"}");
        return 0;
    }
    
    // Handle special case for listing available functions
    if (strcmp(method, "rkllm_list_functions") == 0) {
        return rkllm_proxy_get_functions(result_json);
    }
    
    // Use dynamic proxy to call RKLLM function
    int result = rkllm_proxy_call(method, params_json, result_json);
    
    if (result != 0 && !*result_json) {
        // Provide better error message if proxy call failed
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                 "{\"error\": \"Unknown method or proxy call failed\", \"method\": \"%s\"}", method);
        *result_json = strdup(error_msg);
    }
    
    return result;
}

/**
 * @brief Get current RKLLM handle
 * @return Current handle or NULL if not initialized
 */
LLMHandle io_get_rkllm_handle(void) {
    return rkllm_proxy_get_handle();
}

/**
 * @brief Check if model is initialized
 * @return true if initialized, false otherwise
 */
bool io_is_initialized(void) {
    return rkllm_proxy_get_handle() != NULL;
}


/**
 * @brief Initialize IO operations
 * @return 0 on success, -1 on error
 */
int io_operations_init(void) {
    // Initialize dynamic RKLLM proxy
    return rkllm_proxy_init();
}

/**
 * @brief Shutdown IO operations
 */
void io_operations_shutdown(void) {
    // Clean shutdown of RKLLM via dynamic proxy
    LLMHandle handle = rkllm_proxy_get_handle();
    if (handle) {
        char* result_json = NULL;
        rkllm_proxy_call("rkllm_destroy", "{}", &result_json);
        if (result_json) free(result_json);
    }
    
    // Shutdown dynamic proxy system
    rkllm_proxy_shutdown();
}


// Streaming Support Implementation

bool io_is_streaming_request(const char* params_json) {
    if (!params_json) return false;
    
    json_object* params = json_tokener_parse(params_json);
    if (!params) return false;
    
    json_object* stream_obj;
    bool is_stream = false;
    if (json_object_object_get_ex(params, "stream", &stream_obj)) {
        is_stream = json_object_get_boolean(stream_obj);
    }
    
    json_object_put(params);
    return is_stream;
}



int io_add_stream_chunk(const char* stream_id, const char* delta, bool end, const char* error_msg) {
    return stream_add_chunk(stream_id, delta, end, error_msg);
}

