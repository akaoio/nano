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
    if (!method || !result_json) {
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
 * @brief Parse JSON-RPC request with handle ID (io version)
 * @param json_request JSON-RPC request string
 * @param request_id Output: Request ID from JSON-RPC
 * @param handle_id Output: Handle ID (set to 1 for single model)
 * @param method Output: Method name (32 bytes)
 * @param params Output: Parameters JSON string (4096 bytes)
 * @return 0 on success, -1 on error
 */
int io_parse_json_request_with_handle(const char* json_request, uint32_t* request_id, 
                                      uint32_t* handle_id, char* method, char* params) {
    if (!json_request || !request_id || !handle_id || !method || !params) {
        return -1;
    }
    
    // Initialize outputs
    *request_id = 0;
    *handle_id = 1; // Single model architecture
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
    strncpy(method, json_object_get_string(method_obj), 31);
    method[31] = '\0';
    
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

/**
 * @brief Process JSON request and return response
 * @param json_request JSON request string
 * @param response Output response buffer
 * @param response_size Size of response buffer
 * @return 0 on success, -1 on error
 */
int io_process_request(const char* json_request, char* response, size_t response_size) {
    if (!json_request || !response || response_size == 0) {
        return -1;
    }
    
    // Parse the JSON request
    uint32_t request_id;
    char method[256], params[4096];
    
    if (io_parse_json_request_main(json_request, &request_id, method, params) != 0) {
        snprintf(response, response_size, 
                 "{\"id\":%u,\"error\":{\"code\":-32700,\"message\":\"Parse error\"}}", 
                 request_id);
        return -1;
    }
    
    // Process the operation
    char* result_json = NULL;
    int result = io_process_operation(method, params, &result_json);
    
    if (result == 0 && result_json) {
        // Create success response
        char* final_response = io_create_json_response(request_id, true, result_json);
        if (final_response) {
            strncpy(response, final_response, response_size - 1);
            response[response_size - 1] = '\0';
            free(final_response);
        }
        free(result_json);
    } else {
        // Create error response
        const char* error_msg = result_json ? result_json : "Operation failed";
        char* final_response = io_create_json_response(request_id, false, error_msg);
        if (final_response) {
            strncpy(response, final_response, response_size - 1);
            response[response_size - 1] = '\0';
            free(final_response);
        }
        if (result_json) free(result_json);
    }
    
    return result;
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

int io_streaming_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    io_streaming_context_t* ctx = (io_streaming_context_t*)userdata;
    
    if (!ctx || !ctx->stream_id) return 0;
    
    // Add chunk to stream
    const char* delta = (result && result->text) ? result->text : "";
    bool end = (state == RKLLM_RUN_FINISH);
    const char* error_msg = (state == RKLLM_RUN_ERROR) ? "Stream error" : NULL;
    
    if (strlen(delta) > 0 || end || error_msg) {
        stream_add_chunk(ctx->stream_id, delta, end, error_msg);
        ctx->chunk_count++;
    }
    
    return 0; // Continue streaming
}

int io_handle_stream_run(const char* params_json, char** stream_id, char** result_json) {
    if (!params_json || !stream_id || !result_json) return -1;
    
    *stream_id = NULL;
    *result_json = NULL;
    
    // Check if model is initialized via dynamic proxy
    if (!rkllm_proxy_get_handle()) {
        *result_json = str_copy("{\"error\": \"Model not initialized\"}");
        return -1;
    }
    
    // Parse params and remove stream parameter
    json_object* params = json_tokener_parse(params_json);
    if (!params) return -1;
    
    // Remove stream parameter
    json_object* new_params = json_object_new_object();
    json_object_object_foreach(params, key, val) {
        if (strcmp(key, "stream") != 0) {
            json_object_object_add(new_params, key, json_object_get(val));
        }
    }
    
    // Extract prompt
    json_object* prompt_obj;
    const char* prompt = "";
    if (json_object_object_get_ex(new_params, "prompt", &prompt_obj)) {
        prompt = json_object_get_string(prompt_obj);
    }
    
    // Create stream session
    stream_session_t* session = stream_create_session("rkllm_run", 0);
    if (!session) {
        json_object_put(params);
        json_object_put(new_params);
        return -1;
    }
    
    *stream_id = str_copy(session->stream_id);
    
    // Set up streaming context
    io_streaming_context_t stream_ctx = {0};
    stream_ctx.stream_id = str_copy(session->stream_id);
    stream_ctx.original_method = str_copy("rkllm_run");
    stream_ctx.request_id = 0;
    stream_ctx.stream_started = true;
    stream_ctx.chunk_count = 0;
    
    // Use dynamic proxy for streaming inference
    // Convert params to JSON for dynamic proxy call
    const char* streaming_params_json = json_object_to_json_string(new_params);
    char* proxy_result_json = NULL;
    int result = rkllm_proxy_call("rkllm_run", streaming_params_json, &proxy_result_json);
    
    // Clean up proxy result (streaming handled via callback)
    if (proxy_result_json) free(proxy_result_json);
    
    // Create response
    json_object* response = json_object_new_object();
    json_object_object_add(response, "stream_id", json_object_new_string(session->stream_id));
    json_object_object_add(response, "status", json_object_new_string("streaming_started"));
    
    *result_json = str_copy(json_object_to_json_string(response));
    
    // Cleanup
    json_object_put(params);
    json_object_put(new_params);
    json_object_put(response);
    str_free(stream_ctx.stream_id);
    str_free(stream_ctx.original_method);
    
    return result;
}

int io_add_stream_chunk(const char* stream_id, const char* delta, bool end, const char* error_msg) {
    return stream_add_chunk(stream_id, delta, end, error_msg);
}

int io_process_streaming_request(const char* json_request, char* response, size_t response_size) {
    if (!json_request || !response || response_size == 0) return -1;
    
    uint32_t request_id;
    char method[256];
    char params[4096];
    
    // Parse JSON-RPC request
    int parse_result = io_parse_json_request_main(json_request, &request_id, method, params);
    if (parse_result != 0) {
        char* error_response = io_create_json_response(request_id, false, "Invalid JSON-RPC request");
        if (error_response) {
            strncpy(response, error_response, response_size - 1);
            response[response_size - 1] = '\0';
            free(error_response);
        }
        return -1;
    }
    
    // Check if it's a streaming request
    if (!io_is_streaming_request(params)) {
        // Not a streaming request, use regular processing
        return io_process_request(json_request, response, response_size);
    }
    
    // Handle streaming request
    if (strcmp(method, "rkllm_run") == 0) {
        char* stream_id = NULL;
        char* result_json = NULL;
        
        int result = io_handle_stream_run(params, &stream_id, &result_json);
        
        if (result == 0 && result_json) {
            char* final_response = io_create_json_response(request_id, true, result_json);
            if (final_response) {
                strncpy(response, final_response, response_size - 1);
                response[response_size - 1] = '\0';
                free(final_response);
            }
            free(result_json);
        } else {
            const char* error_msg = result_json ? result_json : "Streaming operation failed";
            char* final_response = io_create_json_response(request_id, false, error_msg);
            if (final_response) {
                strncpy(response, final_response, response_size - 1);
                response[response_size - 1] = '\0';
                free(final_response);
            }
            if (result_json) free(result_json);
        }
        
        str_free(stream_id);
        return result;
    } else {
        // Unsupported streaming method
        char* error_response = io_create_json_response(request_id, false, "Streaming not supported for this method");
        if (error_response) {
            strncpy(response, error_response, response_size - 1);
            response[response_size - 1] = '\0';
            free(error_response);
        }
        return -1;
    }
}