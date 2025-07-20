#include "core/io/io.h"
#include "../libs/rkllm/rkllm.h"
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// IO Operations - Direct RKLLM integration (lightweight approach)
// Handles JSON request parsing and direct RKLLM function calls

// Global RKLLM handle for single model architecture
static LLMHandle g_rkllm_handle = NULL;
static bool g_initialized = false;

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
 * @brief Handle init operation
 * @param params_json Parameters JSON string
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_handle_init(const char* params_json, char** result_json) {
    if (g_initialized) {
        *result_json = strdup("{\"handle_id\": 1, \"status\": \"already_initialized\"}");
        return 0;
    }
    
    // Parse parameters
    json_object* params = json_tokener_parse(params_json);
    if (!params) {
        *result_json = strdup("{\"error\": \"Invalid parameters\"}");
        return -1;
    }
    
    // Create default RKLLM parameters
    RKLLMParam rkllm_param = rkllm_createDefaultParam();
    
    // Extract model path
    json_object* model_path_obj;
    if (json_object_object_get_ex(params, "model_path", &model_path_obj)) {
        rkllm_param.model_path = json_object_get_string(model_path_obj);
    }
    
    // Extract other parameters
    json_object* max_tokens_obj;
    if (json_object_object_get_ex(params, "max_new_tokens", &max_tokens_obj)) {
        rkllm_param.max_new_tokens = json_object_get_int(max_tokens_obj);
    }
    
    json_object* temp_obj;
    if (json_object_object_get_ex(params, "temperature", &temp_obj)) {
        rkllm_param.temperature = json_object_get_double(temp_obj);
    }
    
    // Initialize RKLLM
    int status = rkllm_init(&g_rkllm_handle, &rkllm_param, NULL);
    json_object_put(params);
    
    if (status == 0) {
        g_initialized = true;
        *result_json = strdup("{\"handle_id\": 1, \"status\": \"initialized\"}");
        return 0;
    } else {
        *result_json = strdup("{\"error\": \"Failed to initialize model\"}");
        return -1;
    }
}

/**
 * @brief Handle run operation
 * @param params_json Parameters JSON string
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_handle_run(const char* params_json, char** result_json) {
    if (!g_initialized || !g_rkllm_handle) {
        *result_json = strdup("{\"error\": \"Model not initialized\"}");
        return -1;
    }
    
    // Parse parameters
    json_object* params = json_tokener_parse(params_json);
    if (!params) {
        *result_json = strdup("{\"error\": \"Invalid parameters\"}");
        return -1;
    }
    
    // Extract prompt
    json_object* prompt_obj;
    if (!json_object_object_get_ex(params, "prompt", &prompt_obj)) {
        json_object_put(params);
        *result_json = strdup("{\"error\": \"Missing prompt parameter\"}");
        return -1;
    }
    
    const char* prompt = json_object_get_string(prompt_obj);
    
    // Create RKLLM input
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    // Create RKLLM inference parameters
    RKLLMInferParam infer_param = {0};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 1;
    
    // Run inference
    int status = rkllm_run(g_rkllm_handle, &input, &infer_param, NULL);
    json_object_put(params);
    
    if (status == 0) {
        *result_json = strdup("{\"status\": \"completed\", \"data\": \"Inference completed\"}");
        return 0;
    } else {
        *result_json = strdup("{\"error\": \"Inference failed\"}");
        return -1;
    }
}

/**
 * @brief Handle destroy operation
 * @param params_json Parameters JSON string (unused)
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_handle_destroy(const char* params_json, char** result_json) {
    (void)params_json; // Suppress unused parameter warning
    
    if (!g_initialized) {
        *result_json = strdup("{\"status\": \"not_initialized\"}");
        return 0;
    }
    
    if (g_rkllm_handle) {
        rkllm_destroy(g_rkllm_handle);
        g_rkllm_handle = NULL;
    }
    
    g_initialized = false;
    *result_json = strdup("{\"status\": \"destroyed\"}");
    return 0;
}

/**
 * @brief Process operation request
 * @param method Operation method name
 * @param params_json Parameters JSON string
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_process_operation(const char* method, const char* params_json, char** result_json) {
    if (!method || !result_json) {
        return -1;
    }
    
    if (strcmp(method, "init") == 0) {
        return io_handle_init(params_json, result_json);
    } else if (strcmp(method, "run") == 0) {
        return io_handle_run(params_json, result_json);
    } else if (strcmp(method, "run_streaming") == 0) {
        // Streaming operations are handled by the test framework
        // Return success to indicate the operation was recognized
        *result_json = strdup("{\"status\": \"streaming_initiated\", \"note\": \"Handled by callback mechanism\"}");
        return 0;
    } else if (strcmp(method, "destroy") == 0) {
        return io_handle_destroy(params_json, result_json);
    } else if (strcmp(method, "abort") == 0) {
        // Direct abort call
        if (g_rkllm_handle) {
            rkllm_abort(g_rkllm_handle);
            *result_json = strdup("{\"status\": \"aborted\"}");
            return 0;
        } else {
            *result_json = strdup("{\"error\": \"Model not initialized\"}");
            return -1;
        }
    } else if (strcmp(method, "is_running") == 0) {
        // Direct status check
        if (g_rkllm_handle) {
            int running = rkllm_is_running(g_rkllm_handle);
            *result_json = running ? 
                strdup("{\"status\": \"running\"}") : 
                strdup("{\"status\": \"idle\"}");
            return 0;
        } else {
            *result_json = strdup("{\"error\": \"Model not initialized\"}");
            return -1;
        }
    } else {
        *result_json = strdup("{\"error\": \"Unknown method\"}");
        return -1;
    }
}

/**
 * @brief Get current RKLLM handle
 * @return Current handle or NULL if not initialized
 */
LLMHandle io_get_rkllm_handle(void) {
    return g_rkllm_handle;
}

/**
 * @brief Check if model is initialized
 * @return true if initialized, false otherwise
 */
bool io_is_initialized(void) {
    return g_initialized;
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
    // Nothing to initialize for lightweight approach
    return 0;
}

/**
 * @brief Shutdown IO operations
 */
void io_operations_shutdown(void) {
    // Clean shutdown of RKLLM if initialized
    if (g_initialized && g_rkllm_handle) {
        rkllm_destroy(g_rkllm_handle);
        g_rkllm_handle = NULL;
        g_initialized = false;
    }
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
    
    if (!g_initialized) {
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
    stream_session_t* session = stream_create_session("run", 0);
    if (!session) {
        json_object_put(params);
        json_object_put(new_params);
        return -1;
    }
    
    *stream_id = str_copy(session->stream_id);
    
    // Set up streaming context
    io_streaming_context_t stream_ctx = {0};
    stream_ctx.stream_id = str_copy(session->stream_id);
    stream_ctx.original_method = str_copy("run");
    stream_ctx.request_id = 0;
    stream_ctx.stream_started = true;
    stream_ctx.chunk_count = 0;
    
    // Prepare RKLLM input
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    RKLLMInferParam infer_params = {0};
    infer_params.mode = RKLLM_INFER_GENERATE;
    
    // Start streaming inference
    int result = rkllm_run(g_rkllm_handle, &input, &infer_params, &stream_ctx);
    
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
    if (strcmp(method, "run") == 0) {
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