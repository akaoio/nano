#include "mcp_protocol.h"
#include "common/types.h"
#include <json-c/json.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int mcp_protocol_init(mcp_context_t* ctx) {
    if (!ctx) return -1;
    
    memset(ctx, 0, sizeof(mcp_context_t));
    ctx->state = MCP_STATE_DISCONNECTED;
    ctx->next_request_id = 1;
    
    return 0;
}

void mcp_protocol_shutdown(mcp_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->init_data.protocol_version) {
        free(ctx->init_data.protocol_version);
    }
    if (ctx->init_data.client_info.name) {
        free(ctx->init_data.client_info.name);
    }
    if (ctx->init_data.client_info.version) {
        free(ctx->init_data.client_info.version);
    }
    if (ctx->init_data.server_info.name) {
        free(ctx->init_data.server_info.name);
    }
    if (ctx->init_data.server_info.version) {
        free(ctx->init_data.server_info.version);
    }
    if (ctx->init_data.instructions) {
        free(ctx->init_data.instructions);
    }
    
    memset(ctx, 0, sizeof(mcp_context_t));
}

int mcp_send_initialize(mcp_context_t* ctx, char* buffer, size_t buffer_size) {
    if (!ctx || !buffer || buffer_size == 0) return -1;
    
    if (ctx->state != MCP_STATE_CONNECTED) {
        return -1;
    }
    
    json_object* params = json_object_new_object();
    json_object_object_add(params, "protocolVersion", json_object_new_string(MCP_PROTOCOL_VERSION));
    
    json_object* capabilities = json_object_new_object();
    if (ctx->init_data.capabilities.tools) {
        json_object_object_add(capabilities, "tools", json_object_new_object());
    }
    if (ctx->init_data.capabilities.resources) {
        json_object_object_add(capabilities, "resources", json_object_new_object());
    }
    if (ctx->init_data.capabilities.prompts) {
        json_object_object_add(capabilities, "prompts", json_object_new_object());
    }
    if (ctx->init_data.capabilities.sampling) {
        json_object_object_add(capabilities, "sampling", json_object_new_object());
    }
    json_object_object_add(params, "capabilities", capabilities);
    
    json_object* client_info = json_object_new_object();
    json_object_object_add(client_info, "name", 
        json_object_new_string(ctx->init_data.client_info.name ? ctx->init_data.client_info.name : "nano"));
    json_object_object_add(client_info, "version", 
        json_object_new_string(ctx->init_data.client_info.version ? ctx->init_data.client_info.version : "1.0.0"));
    json_object_object_add(params, "clientInfo", client_info);
    
    const char* params_str = json_object_to_json_string(params);
    int ret = mcp_format_request(ctx->next_request_id++, "initialize", params_str, buffer, buffer_size);
    
    json_object_put(params);
    
    if (ret == 0) {
        ctx->state = MCP_STATE_INITIALIZING;
    }
    
    return ret;
}

int mcp_send_initialized(mcp_context_t* ctx, char* buffer, size_t buffer_size) {
    if (!ctx || !buffer || buffer_size == 0) return -1;
    
    if (ctx->state != MCP_STATE_INITIALIZING) {
        return -1;
    }
    
    int ret = mcp_format_notification("initialized", "{}", buffer, buffer_size);
    
    if (ret == 0) {
        ctx->state = MCP_STATE_INITIALIZED;
    }
    
    return ret;
}

int mcp_send_ping(mcp_context_t* ctx, char* buffer, size_t buffer_size) {
    if (!ctx || !buffer || buffer_size == 0) return -1;
    
    return mcp_format_request(ctx->next_request_id++, "ping", "{}", buffer, buffer_size);
}

int mcp_handle_message(mcp_context_t* ctx, const char* message, char* response, size_t response_size) {
    if (!ctx || !message || !response || response_size == 0) return -1;
    
    json_object* root = json_tokener_parse(message);
    if (!root) {
        return mcp_format_error(0, MCP_ERROR_PARSE, "Parse error", NULL, response, response_size);
    }
    
    json_object* jsonrpc_obj;
    if (!json_object_object_get_ex(root, "jsonrpc", &jsonrpc_obj) || 
        strcmp(json_object_get_string(jsonrpc_obj), MCP_JSONRPC_VERSION) != 0) {
        json_object_put(root);
        return mcp_format_error(0, MCP_ERROR_INVALID_REQUEST, "Invalid JSON-RPC version", NULL, response, response_size);
    }
    
    json_object* id_obj;
    bool has_id = json_object_object_get_ex(root, "id", &id_obj);
    uint32_t id = has_id ? (uint32_t)json_object_get_int(id_obj) : 0;
    
    json_object* method_obj;
    if (json_object_object_get_ex(root, "method", &method_obj)) {
        const char* method = json_object_get_string(method_obj);
        
        json_object* params_obj;
        const char* params_str = "{}";
        if (json_object_object_get_ex(root, "params", &params_obj)) {
            params_str = json_object_to_json_string(params_obj);
        }
        
        if (strcmp(method, "ping") == 0) {
            json_object_put(root);
            return mcp_format_response(id, "{}", response, response_size);
        }
        
        if (ctx->state < MCP_STATE_INITIALIZED && strcmp(method, "initialize") != 0) {
            json_object_put(root);
            return mcp_format_error(id, MCP_ERROR_NOT_INITIALIZED, "Not initialized", NULL, response, response_size);
        }
        
        if (has_id && ctx->on_request) {
            char result[4096];
            int ret = ctx->on_request(method, params_str, result, sizeof(result));
            json_object_put(root);
            
            if (ret == 0) {
                return mcp_format_response(id, result, response, response_size);
            } else {
                return mcp_format_error(id, MCP_ERROR_INTERNAL, "Request handler failed", NULL, response, response_size);
            }
        } else if (!has_id && ctx->on_notification) {
            ctx->on_notification(method, params_str);
            json_object_put(root);
            response[0] = '\0';
            return 0;
        } else {
            json_object_put(root);
            return mcp_format_error(id, MCP_ERROR_METHOD_NOT_FOUND, "Method not found", NULL, response, response_size);
        }
    }
    
    json_object_put(root);
    return mcp_format_error(id, MCP_ERROR_INVALID_REQUEST, "Invalid request", NULL, response, response_size);
}

int mcp_format_request(uint32_t id, const char* method, const char* params, char* buffer, size_t buffer_size) {
    if (!method || !buffer || buffer_size == 0) return -1;
    
    json_object* msg = json_object_new_object();
    json_object_object_add(msg, "jsonrpc", json_object_new_string(MCP_JSONRPC_VERSION));
    json_object_object_add(msg, "id", json_object_new_int(id));
    json_object_object_add(msg, "method", json_object_new_string(method));
    
    if (params && strcmp(params, "{}") != 0) {
        json_object* params_obj = json_tokener_parse(params);
        if (params_obj) {
            json_object_object_add(msg, "params", params_obj);
        }
    }
    
    const char* json_str = json_object_to_json_string(msg);
    strncpy(buffer, json_str, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    json_object_put(msg);
    return 0;
}

int mcp_format_response(uint32_t id, const char* result, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return -1;
    
    json_object* msg = json_object_new_object();
    json_object_object_add(msg, "jsonrpc", json_object_new_string(MCP_JSONRPC_VERSION));
    json_object_object_add(msg, "id", json_object_new_int(id));
    
    if (result) {
        json_object* result_obj = json_tokener_parse(result);
        if (result_obj) {
            json_object_object_add(msg, "result", result_obj);
        } else {
            json_object_object_add(msg, "result", json_object_new_string(result));
        }
    } else {
        json_object_object_add(msg, "result", json_object_new_object());
    }
    
    const char* json_str = json_object_to_json_string(msg);
    strncpy(buffer, json_str, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    json_object_put(msg);
    return 0;
}

int mcp_format_error(uint32_t id, mcp_error_code_t code, const char* message, const char* data, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return -1;
    
    json_object* msg = json_object_new_object();
    json_object_object_add(msg, "jsonrpc", json_object_new_string(MCP_JSONRPC_VERSION));
    json_object_object_add(msg, "id", json_object_new_int(id));
    
    json_object* error = json_object_new_object();
    json_object_object_add(error, "code", json_object_new_int(code));
    json_object_object_add(error, "message", json_object_new_string(message ? message : mcp_error_message(code)));
    
    if (data) {
        json_object* data_obj = json_tokener_parse(data);
        if (data_obj) {
            json_object_object_add(error, "data", data_obj);
        } else {
            json_object_object_add(error, "data", json_object_new_string(data));
        }
    }
    
    json_object_object_add(msg, "error", error);
    
    const char* json_str = json_object_to_json_string(msg);
    strncpy(buffer, json_str, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    json_object_put(msg);
    return 0;
}

int mcp_format_notification(const char* method, const char* params, char* buffer, size_t buffer_size) {
    if (!method || !buffer || buffer_size == 0) return -1;
    
    json_object* msg = json_object_new_object();
    json_object_object_add(msg, "jsonrpc", json_object_new_string(MCP_JSONRPC_VERSION));
    json_object_object_add(msg, "method", json_object_new_string(method));
    
    if (params && strcmp(params, "{}") != 0) {
        json_object* params_obj = json_tokener_parse(params);
        if (params_obj) {
            json_object_object_add(msg, "params", params_obj);
        }
    }
    
    const char* json_str = json_object_to_json_string(msg);
    strncpy(buffer, json_str, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    json_object_put(msg);
    return 0;
}

// Streaming Support Functions
int mcp_format_stream_chunk(const char* method, const char* stream_id, uint32_t seq, const char* delta, bool end, const char* error_msg, char* buffer, size_t buffer_size) {
    if (!method || !stream_id || !buffer || buffer_size == 0) return -1;
    
    json_object* msg = json_object_new_object();
    json_object_object_add(msg, "jsonrpc", json_object_new_string(MCP_JSONRPC_VERSION));
    json_object_object_add(msg, "method", json_object_new_string(method));
    
    json_object* params = json_object_new_object();
    json_object_object_add(params, "stream_id", json_object_new_string(stream_id));
    json_object_object_add(params, "seq", json_object_new_int(seq));
    json_object_object_add(params, "delta", json_object_new_string(delta ? delta : ""));
    json_object_object_add(params, "end", json_object_new_boolean(end));
    
    if (error_msg) {
        json_object* error_obj = json_object_new_object();
        json_object_object_add(error_obj, "message", json_object_new_string(error_msg));
        json_object_object_add(params, "error", error_obj);
    }
    
    json_object_object_add(msg, "params", params);
    
    const char* json_str = json_object_to_json_string(msg);
    strncpy(buffer, json_str, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    json_object_put(msg);
    return 0;
}

int mcp_parse_stream_request(const char* params, bool* is_stream, char* other_params, size_t params_size) {
    if (!params || !is_stream || !other_params || params_size == 0) return -1;
    
    *is_stream = false;
    other_params[0] = '\0';
    
    json_object* params_obj = json_tokener_parse(params);
    if (!params_obj) {
        strncpy(other_params, params, params_size - 1);
        other_params[params_size - 1] = '\0';
        return 0;
    }
    
    // Check for stream parameter
    json_object* stream_obj;
    if (json_object_object_get_ex(params_obj, "stream", &stream_obj)) {
        *is_stream = json_object_get_boolean(stream_obj);
        
        // Remove stream parameter and create new params object
        json_object* new_params = json_object_new_object();
        
        json_object_object_foreach(params_obj, key, val) {
            if (strcmp(key, "stream") != 0) {
                json_object_object_add(new_params, key, json_object_get(val));
            }
        }
        
        const char* new_params_str = json_object_to_json_string(new_params);
        strncpy(other_params, new_params_str, params_size - 1);
        other_params[params_size - 1] = '\0';
        
        json_object_put(new_params);
    } else {
        // No stream parameter, copy original params
        strncpy(other_params, params, params_size - 1);
        other_params[params_size - 1] = '\0';
    }
    
    json_object_put(params_obj);
    return 0;
}

int mcp_handle_stream_poll_request(const char* stream_id, uint32_t from_seq, char* response, size_t response_size) {
    if (!stream_id || !response || response_size == 0) return -1;
    
    // This is a placeholder - actual implementation would interface with stream manager
    // For now, return a "not implemented" error
    json_object* error = json_object_new_object();
    json_object_object_add(error, "code", json_object_new_int(MCP_ERROR_METHOD_NOT_FOUND));
    json_object_object_add(error, "message", json_object_new_string("Stream polling not implemented at protocol level"));
    
    const char* error_str = json_object_to_json_string(error);
    strncpy(response, error_str, response_size - 1);
    response[response_size - 1] = '\0';
    
    json_object_put(error);
    return -1;
}

const char* mcp_error_message(mcp_error_code_t code) {
    switch (code) {
        case MCP_ERROR_PARSE: return "Parse error";
        case MCP_ERROR_INVALID_REQUEST: return "Invalid request";
        case MCP_ERROR_METHOD_NOT_FOUND: return "Method not found";
        case MCP_ERROR_INVALID_PARAMS: return "Invalid params";
        case MCP_ERROR_INTERNAL: return "Internal error";
        case MCP_ERROR_NOT_INITIALIZED: return "Not initialized";
        case MCP_ERROR_ALREADY_INITIALIZED: return "Already initialized";
        case MCP_ERROR_INVALID_VERSION: return "Invalid protocol version";
        case MCP_ERROR_STREAM_NOT_FOUND: return "Stream session not found or expired";
        case MCP_ERROR_STREAM_EXPIRED: return "Stream session expired";
        case MCP_ERROR_STREAM_INVALID_STATE: return "Stream in invalid state";
        default: return "Unknown error";
    }
}