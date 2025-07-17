#include "mcp_base.h"
#include "../../../common/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mcp_base_init(void) {
    // Initialize MCP base subsystem
    return 0;
}

void mcp_base_shutdown(void) {
    // Cleanup MCP base subsystem
}

int mcp_message_create(mcp_message_t* msg, mcp_message_type_t type, uint32_t id, const char* method, const char* params) {
    if (!msg) return -1;
    
    msg->type = type;
    msg->id = id;
    msg->method = method ? str_copy(method) : NULL;
    msg->params = params ? str_copy(params) : NULL;
    msg->params_len = params ? strlen(params) : 0;
    
    return 0;
}

void mcp_message_destroy(mcp_message_t* msg) {
    if (!msg) return;
    
    str_free(msg->method);
    str_free(msg->params);
    
    msg->method = NULL;
    msg->params = NULL;
    msg->params_len = 0;
}

int mcp_parse_json_rpc(const char* json, mcp_message_t* msg) {
    if (!json || !msg) return -1;
    
    // Simple JSON-RPC parsing
    char id_str[32];
    char method[256];
    char params[4096];
    
    // Check if it's a response (has "result" or "error")
    if (strstr(json, "\"result\"") || strstr(json, "\"error\"")) {
        msg->type = MCP_RESPONSE;
        msg->method = NULL;
        
        // Extract ID
        if (json_get_string(json, "id", id_str, sizeof(id_str))) {
            msg->id = atoi(id_str);
        }
        
        // Extract result or error
        if (json_get_string(json, "result", params, sizeof(params))) {
            msg->params = str_copy(params);
            msg->params_len = strlen(params);
        } else if (json_get_string(json, "error", params, sizeof(params))) {
            msg->params = str_copy(params);
            msg->params_len = strlen(params);
        }
    } else {
        // It's a request or notification
        if (json_get_string(json, "id", id_str, sizeof(id_str))) {
            msg->type = MCP_REQUEST;
            msg->id = atoi(id_str);
        } else {
            msg->type = MCP_NOTIFICATION;
            msg->id = 0;
        }
        
        // Extract method
        if (json_get_string(json, "method", method, sizeof(method))) {
            msg->method = str_copy(method);
        }
        
        // Extract params
        if (json_get_string(json, "params", params, sizeof(params))) {
            msg->params = str_copy(params);
            msg->params_len = strlen(params);
        }
    }
    
    return 0;
}

int mcp_format_json_rpc(const mcp_message_t* msg, char* buffer, size_t buffer_size) {
    if (!msg || !buffer || buffer_size == 0) return -1;
    
    switch (msg->type) {
        case MCP_REQUEST:
            snprintf(buffer, buffer_size,
                    "{\"jsonrpc\":\"2.0\",\"id\":%u,\"method\":\"%s\",\"params\":%s}",
                    msg->id, msg->method ? msg->method : "", msg->params ? msg->params : "{}");
            break;
            
        case MCP_RESPONSE:
            snprintf(buffer, buffer_size,
                    "{\"jsonrpc\":\"2.0\",\"id\":%u,\"result\":%s}",
                    msg->id, msg->params ? msg->params : "{}");
            break;
            
        case MCP_NOTIFICATION:
            snprintf(buffer, buffer_size,
                    "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s}",
                    msg->method ? msg->method : "", msg->params ? msg->params : "{}");
            break;
    }
    
    return 0;
}
