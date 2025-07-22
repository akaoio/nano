#include "adapter.h"
#include "http_buffer_manager.h"
#include "mcp_protocol.h"
#include "../core/settings_global.h"
#include "../core/stream_manager.h"
#include "common/types.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global MCP adapter instance
mcp_adapter_t g_mcp_adapter = {0};

int mcp_adapter_init(mcp_adapter_t* adapter) {
    if (!adapter) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    memset(adapter, 0, sizeof(mcp_adapter_t));
    strncpy(adapter->protocol_version, "2025-03-26", sizeof(adapter->protocol_version) - 1);
    adapter->utf8_validation_enabled = true;
    adapter->message_batching_enabled = true;
    
    // Initialize stream manager
    stream_manager_t* stream_manager = get_global_stream_manager();
    if (stream_manager_init(stream_manager) != 0) {
        return MCP_ADAPTER_ERROR_STREAM_ERROR;
    }
    
    // Initialize HTTP buffer manager
    adapter->http_buffers = malloc(sizeof(http_buffer_manager_t));
    if (!adapter->http_buffers) {
        stream_manager_shutdown(stream_manager);
        return MCP_ADAPTER_ERROR_STREAM_ERROR;
    }
    
    if (http_buffer_manager_init(adapter->http_buffers) != 0) {
        free(adapter->http_buffers);
        adapter->http_buffers = NULL;
        stream_manager_shutdown(stream_manager);
        return MCP_ADAPTER_ERROR_STREAM_ERROR;
    }
    
    // Initialize IO operations (RKLLM proxy)
    if (io_operations_init() != 0) {
        http_buffer_manager_shutdown(adapter->http_buffers);
        free(adapter->http_buffers);
        adapter->http_buffers = NULL;
        stream_manager_shutdown(stream_manager);
        return MCP_ADAPTER_ERROR_INVALID_JSON;
    }
    
    adapter->initialized = true;
    printf("MCP Adapter: Initialized with HTTP buffer manager\n");
    return MCP_ADAPTER_OK;
}

void mcp_adapter_shutdown(mcp_adapter_t* adapter) {
    if (!adapter || !adapter->initialized) return;
    
    printf("MCP Adapter: Shutting down...\n");
    
    // Shutdown HTTP buffer manager
    if (adapter->http_buffers) {
        http_buffer_manager_shutdown(adapter->http_buffers);
        free(adapter->http_buffers);
        adapter->http_buffers = NULL;
    }
    
    // Shutdown stream manager
    stream_manager_t* stream_manager = get_global_stream_manager();
    stream_manager_shutdown(stream_manager);
    
    io_operations_shutdown();
    adapter->initialized = false;
    printf("MCP Adapter: Shutdown complete\n");
}

// UTF-8 Validation - Single implementation for all transports
int mcp_adapter_validate_utf8(const char* data) {
    if (!data) return MCP_ADAPTER_ERROR_INVALID_UTF8;
    
    const unsigned char* bytes = (const unsigned char*)data;
    while (*bytes) {
        if (*bytes < 0x80) {
            bytes++;
        } else if ((*bytes >> 5) == 0x06) {
            if ((bytes[1] & 0xC0) != 0x80) return MCP_ADAPTER_ERROR_INVALID_UTF8;
            bytes += 2;
        } else if ((*bytes >> 4) == 0x0E) {
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80) return MCP_ADAPTER_ERROR_INVALID_UTF8;
            bytes += 3;
        } else if ((*bytes >> 3) == 0x1E) {
            if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80 || (bytes[3] & 0xC0) != 0x80) return MCP_ADAPTER_ERROR_INVALID_UTF8;
            bytes += 4;
        } else {
            return MCP_ADAPTER_ERROR_INVALID_UTF8;
        }
    }
    return MCP_ADAPTER_OK;
}

// JSON-RPC Validation - Single implementation
int mcp_adapter_validate_json_rpc(const char* data) {
    if (!data) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    json_object* root = json_tokener_parse(data);
    if (!root) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    // Check required JSON-RPC 2.0 fields
    json_object* jsonrpc_obj;
    if (!json_object_object_get_ex(root, "jsonrpc", &jsonrpc_obj) ||
        strcmp(json_object_get_string(jsonrpc_obj), "2.0") != 0) {
        json_object_put(root);
        return MCP_ADAPTER_ERROR_INVALID_JSON;
    }
    
    // Must have either method (request) or result/error (response)
    json_object* method_obj, *result_obj, *error_obj;
    bool has_method = json_object_object_get_ex(root, "method", &method_obj);
    bool has_result = json_object_object_get_ex(root, "result", &result_obj);
    bool has_error = json_object_object_get_ex(root, "error", &error_obj);
    
    if (!has_method && !has_result && !has_error) {
        json_object_put(root);
        return MCP_ADAPTER_ERROR_MISSING_FIELD;
    }
    
    json_object_put(root);
    return MCP_ADAPTER_OK;
}

// Request Parsing - Single implementation for all transports
int mcp_adapter_parse_request(const char* raw_data, mcp_request_t* request) {
    if (!raw_data || !request) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    memset(request, 0, sizeof(mcp_request_t));
    
    // Validate UTF-8 if enabled
    if (g_mcp_adapter.utf8_validation_enabled) {
        if (mcp_adapter_validate_utf8(raw_data) != MCP_ADAPTER_OK) {
            return MCP_ADAPTER_ERROR_INVALID_UTF8;
        }
    }
    
    // Validate JSON-RPC structure
    if (mcp_adapter_validate_json_rpc(raw_data) != MCP_ADAPTER_OK) {
        return MCP_ADAPTER_ERROR_INVALID_JSON;
    }
    
    json_object* root = json_tokener_parse(raw_data);
    if (!root) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    // Extract request ID
    json_object* id_obj;
    if (json_object_object_get_ex(root, "id", &id_obj)) {
        snprintf(request->request_id, sizeof(request->request_id), "%d", json_object_get_int(id_obj));
    }
    
    // Extract method
    json_object* method_obj;
    if (json_object_object_get_ex(root, "method", &method_obj)) {
        strncpy(request->method, json_object_get_string(method_obj), sizeof(request->method) - 1);
    } else {
        json_object_put(root);
        return MCP_ADAPTER_ERROR_MISSING_FIELD;
    }
    
    // Extract params
    json_object* params_obj;
    if (json_object_object_get_ex(root, "params", &params_obj)) {
        const char* params_str = json_object_to_json_string(params_obj);
        strncpy(request->params, params_str, sizeof(request->params) - 1);
        
        // Check for streaming parameter
        json_object* stream_obj;
        if (json_object_object_get_ex(params_obj, "stream", &stream_obj)) {
            request->is_streaming = json_object_get_boolean(stream_obj);
        }
    }
    
    json_object_put(root);
    g_mcp_adapter.requests_processed++;
    return MCP_ADAPTER_OK;
}

// Request Validation
int mcp_adapter_validate_request(const mcp_request_t* request) {
    if (!request) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    // Validate method name
    if (strlen(request->method) == 0) {
        return MCP_ADAPTER_ERROR_INVALID_METHOD;
    }
    
    // Add more validation as needed
    return MCP_ADAPTER_OK;
}

// Request Processing - Delegates to IO operations
int mcp_adapter_process_request(const mcp_request_t* request, mcp_response_t* response) {
    if (!request || !response) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    memset(response, 0, sizeof(mcp_response_t));
    strncpy(response->request_id, request->request_id, sizeof(response->request_id) - 1);
    
    // Handle streaming requests
    if (request->is_streaming) {
        return mcp_adapter_handle_stream_request(request, response);
    }
    
    // Handle NPU status polling requests
    if (strcmp(request->method, "npu_status") == 0) {
        char status_response[4096];
        int status_result = mcp_handle_npu_status_request(request->request_id, status_response, sizeof(status_response));
        
        if (status_result == 0) {
            response->is_success = true;
            strncpy(response->result, status_response, sizeof(response->result) - 1);
        } else {
            response->is_success = false;
            strncpy(response->error_code, "-32002", sizeof(response->error_code) - 1);
            strncpy(response->error_message, "NPU status request failed", sizeof(response->error_message) - 1);
        }
        return MCP_ADAPTER_OK;
    }
    
    // Regular request processing via IO operations
    char* result_json = NULL;
    int io_result = io_process_operation(request->method, request->params, &result_json);
    
    if (io_result == 0 && result_json) {
        response->is_success = true;
        strncpy(response->result, result_json, sizeof(response->result) - 1);
        free(result_json);
    } else {
        response->is_success = false;
        strncpy(response->error_code, "-32603", sizeof(response->error_code) - 1);
        strncpy(response->error_message, result_json ? result_json : "Internal error", sizeof(response->error_message) - 1);
        if (result_json) free(result_json);
    }
    
    return MCP_ADAPTER_OK;
}

// Response Formatting - Single implementation
int mcp_adapter_format_response(const mcp_response_t* response, char* output, size_t output_size) {
    if (!response || !output || output_size == 0) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    json_object* root = json_object_new_object();
    json_object_object_add(root, "jsonrpc", json_object_new_string("2.0"));
    
    // Add request ID if present
    if (strlen(response->request_id) > 0) {
        json_object_object_add(root, "id", json_object_new_int(atoi(response->request_id)));
    }
    
    if (response->is_success) {
        // Success response
        json_object* result_obj = json_tokener_parse(response->result);
        if (result_obj) {
            json_object_object_add(root, "result", result_obj);
        } else {
            json_object_object_add(root, "result", json_object_new_string(response->result));
        }
    } else {
        // Error response
        json_object* error_obj = json_object_new_object();
        json_object_object_add(error_obj, "code", json_object_new_int(atoi(response->error_code)));
        json_object_object_add(error_obj, "message", json_object_new_string(response->error_message));
        json_object_object_add(root, "error", error_obj);
    }
    
    const char* json_str = json_object_to_json_string(root);
    strncpy(output, json_str, output_size - 1);
    output[output_size - 1] = '\0';
    
    json_object_put(root);
    g_mcp_adapter.responses_sent++;
    return MCP_ADAPTER_OK;
}

// Error Response Formatting
int mcp_adapter_format_error(const char* request_id, int error_code, const char* message, char* output, size_t output_size) {
    if (!output || output_size == 0) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    json_object* root = json_object_new_object();
    json_object_object_add(root, "jsonrpc", json_object_new_string("2.0"));
    
    if (request_id && strlen(request_id) > 0) {
        json_object_object_add(root, "id", json_object_new_int(atoi(request_id)));
    }
    
    json_object* error_obj = json_object_new_object();
    json_object_object_add(error_obj, "code", json_object_new_int(error_code));
    json_object_object_add(error_obj, "message", json_object_new_string(message ? message : "Internal error"));
    json_object_object_add(root, "error", error_obj);
    
    const char* json_str = json_object_to_json_string(root);
    strncpy(output, json_str, output_size - 1);
    output[output_size - 1] = '\0';
    
    json_object_put(root);
    g_mcp_adapter.errors_handled++;
    return MCP_ADAPTER_OK;
}

// Streaming Support - Single implementation
int mcp_adapter_create_stream(const char* method, const char* request_id) {
    if (!method || !request_id) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    // No need to create separate stream ID - just use request_id directly
    return MCP_ADAPTER_OK;
}

int mcp_adapter_handle_stream_request(const mcp_request_t* request, mcp_response_t* response) {
    if (!request || !response) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    // Format stream initialization response using request_id
    json_object* result = json_object_new_object();
    json_object_object_add(result, "status", json_object_new_string("streaming_started"));
    
    const char* result_str = json_object_to_json_string(result);
    strncpy(response->result, result_str, sizeof(response->result) - 1);
    response->is_success = true;
    response->is_streaming_response = true;
    response->request_id_ref = strdup(request->request_id);
    
    json_object_put(result);
    return MCP_ADAPTER_OK;
}

int mcp_adapter_format_stream_chunk(const mcp_stream_chunk_t* chunk, char* output, size_t output_size) {
    if (!chunk || !output || output_size == 0) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    // CRITICAL FIX: Stream chunks must be JSON-RPC responses with original request_id
    // NOT notifications - they need the "id" field to match the original request
    json_object* root = json_object_new_object();
    json_object_object_add(root, "jsonrpc", json_object_new_string("2.0"));
    
    // Add original request_id - this is the key fix
    if (chunk->request_id[0] != '\0') {
        json_object_object_add(root, "id", json_object_new_int(atoi(chunk->request_id)));
    }
    
    // Wrap chunk data in "result" object (proper JSON-RPC response format)
    json_object* result = json_object_new_object();
    json_object* chunk_data = json_object_new_object();
    
    json_object_object_add(chunk_data, "seq", json_object_new_int(chunk->seq));
    json_object_object_add(chunk_data, "delta", json_object_new_string(chunk->delta));
    
    // Only add "end" field when true (as per spec)
    if (chunk->end) {
        json_object_object_add(chunk_data, "end", json_object_new_boolean(true));
    }
    
    if (chunk->error_message) {
        json_object* error_obj = json_object_new_object();
        json_object_object_add(error_obj, "message", json_object_new_string(chunk->error_message));
        json_object_object_add(chunk_data, "error", error_obj);
    }
    
    json_object_object_add(result, "chunk", chunk_data);
    json_object_object_add(root, "result", result);
    
    const char* json_str = json_object_to_json_string(root);
    strncpy(output, json_str, output_size - 1);
    output[output_size - 1] = '\0';
    
    json_object_put(root);
    g_mcp_adapter.stream_chunks_sent++;
    return MCP_ADAPTER_OK;
}

// Message Batching Support
int mcp_adapter_parse_batch(const char* raw_data, mcp_request_t* requests, size_t* count, size_t max_count) {
    if (!raw_data || !requests || !count || max_count == 0) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    *count = 0;
    
    json_object* root = json_tokener_parse(raw_data);
    if (!root) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    if (!json_object_is_type(root, json_type_array)) {
        // Single message, not a batch
        json_object_put(root);
        return mcp_adapter_parse_request(raw_data, &requests[0]) == MCP_ADAPTER_OK ? 1 : MCP_ADAPTER_ERROR_INVALID_JSON;
    }
    
    size_t array_len = json_object_array_length(root);
    if (array_len > max_count) array_len = max_count;
    
    for (size_t i = 0; i < array_len; i++) {
        json_object* msg_obj = json_object_array_get_idx(root, i);
        if (msg_obj) {
            const char* msg_str = json_object_to_json_string(msg_obj);
            if (mcp_adapter_parse_request(msg_str, &requests[i]) == MCP_ADAPTER_OK) {
                (*count)++;
            }
        }
    }
    
    json_object_put(root);
    return *count > 0 ? MCP_ADAPTER_OK : MCP_ADAPTER_ERROR_INVALID_JSON;
}

int mcp_adapter_format_batch_response(const mcp_response_t* responses, size_t count, char* output, size_t output_size) {
    if (!responses || count == 0 || !output || output_size == 0) return MCP_ADAPTER_ERROR_INVALID_JSON;
    
    if (count == 1) {
        // Single response, not a batch
        return mcp_adapter_format_response(&responses[0], output, output_size);
    }
    
    json_object* batch_array = json_object_new_array();
    
    for (size_t i = 0; i < count; i++) {
        size_t buffer_size = SETTING_BUFFER(response_buffer_size);
        if (buffer_size == 0) buffer_size = 4096; // fallback
        char* response_buffer = malloc(buffer_size);
        if (!response_buffer) continue;
        
        if (mcp_adapter_format_response(&responses[i], response_buffer, buffer_size) == MCP_ADAPTER_OK) {
            json_object* response_obj = json_tokener_parse(response_buffer);
            if (response_obj) {
                json_object_array_add(batch_array, response_obj);
            }
        }
        free(response_buffer);
    }
    
    const char* batch_str = json_object_to_json_string(batch_array);
    strncpy(output, batch_str, output_size - 1);
    output[output_size - 1] = '\0';
    
    json_object_put(batch_array);
    return MCP_ADAPTER_OK;
}

// Get global HTTP buffer manager
struct http_buffer_manager_t* get_global_http_buffer_manager(void) {
    return g_mcp_adapter.http_buffers;
}