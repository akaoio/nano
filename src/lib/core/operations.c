#include "operations.h"
#include "rkllm_proxy.h"
#include "streaming_integration.h"
#include "memory_management_operations.h"
#include "performance_operations.h"
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
    
    // Handle memory management operations
    if (strncmp(method, "memory_", 7) == 0) {
        if (strcmp(method, "memory_get_statistics") == 0) {
            return memory_operation_get_statistics(params_json, result_json);
        } else if (strcmp(method, "memory_check_leaks") == 0) {
            return memory_operation_check_leaks(params_json, result_json);
        } else if (strcmp(method, "memory_get_pool_stats") == 0) {
            return memory_operation_get_pool_stats(params_json, result_json);
        } else if (strcmp(method, "memory_garbage_collect") == 0) {
            return memory_operation_garbage_collect(params_json, result_json);
        } else if (strcmp(method, "memory_set_pressure_threshold") == 0) {
            return memory_operation_set_pressure_threshold(params_json, result_json);
        } else if (strcmp(method, "memory_validate_integrity") == 0) {
            return memory_operation_validate_integrity(params_json, result_json);
        } else if (strcmp(method, "memory_get_allocation_breakdown") == 0) {
            return memory_operation_get_allocation_breakdown(params_json, result_json);
        } else {
            *result_json = strdup("{\"error\": \"Unknown memory management operation\"}");
            return -1;
        }
    }
    
    // Handle performance monitoring operations
    if (strncmp(method, "performance_", 12) == 0) {
        if (strcmp(method, "performance_get_statistics") == 0) {
            return performance_operation_get_statistics(params_json, result_json);
        } else if (strcmp(method, "performance_generate_report") == 0) {
            return performance_operation_generate_report(params_json, result_json);
        } else if (strcmp(method, "performance_create_counter") == 0) {
            return performance_operation_create_counter(params_json, result_json);
        } else if (strcmp(method, "performance_create_timer") == 0) {
            return performance_operation_create_timer(params_json, result_json);
        } else if (strcmp(method, "performance_start_timer") == 0) {
            return performance_operation_start_timer(params_json, result_json);
        } else if (strcmp(method, "performance_stop_timer") == 0) {
            return performance_operation_stop_timer(params_json, result_json);
        } else if (strcmp(method, "performance_reset_metrics") == 0) {
            return performance_operation_reset_metrics(params_json, result_json);
        } else if (strcmp(method, "performance_get_system_resources") == 0) {
            return performance_operation_get_system_resources(params_json, result_json);
        } else if (strcmp(method, "performance_configure_monitoring") == 0) {
            return performance_operation_configure_monitoring(params_json, result_json);
        } else {
            *result_json = strdup("{\"error\": \"Unknown performance monitoring operation\"}");
            return -1;
        }
    }
    
    // Handle streaming integration methods
    if (strncmp(method, "streaming_", 10) == 0) {
        // Delegate to streaming integration system
        // Note: This would need transport manager context in real usage
        streaming_integration_result_t stream_result = streaming_integration_handle_request(
            method, params_json, 0, NULL, result_json);
        return (stream_result == STREAMING_INTEGRATION_OK) ? 0 : -1;
    }
    
    // Special handling for legacy streaming requests
    if (strcmp(method, "rkllm_run_streaming") == 0) {
        // Enhanced streaming - create a basic streaming session for backward compatibility
        *result_json = strdup("{\"status\": \"streaming_initiated\", \"note\": \"Use streaming_create_session for full featured streaming\", \"enhanced_streaming_available\": true}");
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
    int result = rkllm_proxy_init();
    if (result != 0) {
        return result;
    }
    
    // Initialize streaming integration system
    streaming_integration_result_t stream_result = streaming_integration_init();
    if (stream_result != STREAMING_INTEGRATION_OK) {
        printf("⚠️  Streaming integration initialization failed, continuing without advanced streaming\n");
        // Continue without failing - basic operations still work
    }
    
    return 0;
}

/**
 * @brief Shutdown IO operations
 */
void io_operations_shutdown(void) {
    // Shutdown streaming integration system first
    streaming_integration_shutdown();
    
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

