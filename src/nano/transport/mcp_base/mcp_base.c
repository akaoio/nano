#include "mcp_base.h"
#include "../../../common/core.h"
#include <json-c/json.h>
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
    msg->method = method ? str_copy(method) : nullptr;
    msg->params = params ? str_copy(params) : nullptr;
    msg->params_len = params ? strlen(params) : 0;
    
    return 0;
}

void mcp_message_destroy(mcp_message_t* msg) {
    if (!msg) return;
    
    str_free(msg->method);
    str_free(msg->params);
    
    msg->method = nullptr;
    msg->params = nullptr;
    msg->params_len = 0;
}

int mcp_parse_json_rpc(const char* json, mcp_message_t* msg) {
    if (!json || !msg) return -1;
    
    // Parse JSON using json-c
    json_object *root = json_tokener_parse(json);
    if (!root) return -1;
    
    // Initialize message
    msg->params = nullptr;
    msg->params_len = 0;
    msg->method = nullptr;
    msg->id = 0;
    
    // Check if it's a response (has "result" or "error")
    json_object *result_obj, *error_obj;
    bool has_result = json_object_object_get_ex(root, "result", &result_obj);
    bool has_error = json_object_object_get_ex(root, "error", &error_obj);
    
    if (has_result || has_error) {
        msg->type = MCP_RESPONSE;
        
        // Extract ID
        json_object *id_obj;
        if (json_object_object_get_ex(root, "id", &id_obj)) {
            msg->id = (uint32_t)json_object_get_int(id_obj);
        }
        
        // Extract result or error
        if (has_result) {
            const char *result_str = json_object_to_json_string(result_obj);
            if (result_str) {
                msg->params = str_copy(result_str);
                msg->params_len = strlen(result_str);
            }
        } else if (has_error) {
            const char *error_str = json_object_to_json_string(error_obj);
            if (error_str) {
                msg->params = str_copy(error_str);
                msg->params_len = strlen(error_str);
            }
        }
    } else {
        // It's a request or notification
        json_object *id_obj;
        if (json_object_object_get_ex(root, "id", &id_obj)) {
            msg->type = MCP_REQUEST;
            msg->id = (uint32_t)json_object_get_int(id_obj);
        } else {
            msg->type = MCP_NOTIFICATION;
            msg->id = 0;
        }
        
        // Extract method
        json_object *method_obj;
        if (json_object_object_get_ex(root, "method", &method_obj)) {
            const char *method_str = json_object_get_string(method_obj);
            if (method_str) {
                msg->method = str_copy(method_str);
            }
        }
        
        // Extract params
        json_object *params_obj;
        if (json_object_object_get_ex(root, "params", &params_obj)) {
            const char *params_str = json_object_to_json_string(params_obj);
            if (params_str) {
                msg->params = str_copy(params_str);
                msg->params_len = strlen(params_str);
            }
        }
    }
    
    json_object_put(root);
    return 0;
}

int mcp_format_json_rpc(const mcp_message_t* msg, char* buffer, size_t buffer_size) {
    if (!msg || !buffer || buffer_size == 0) return -1;
    
    json_object *json_msg = json_object_new_object();
    json_object *jsonrpc = json_object_new_string("2.0");
    json_object_object_add(json_msg, "jsonrpc", jsonrpc);
    
    switch (msg->type) {
        case MCP_REQUEST: {
            json_object *id = json_object_new_int(msg->id);
            json_object *method = json_object_new_string(msg->method ? msg->method : "");
            json_object *params;
            
            if (msg->params) {
                params = json_tokener_parse(msg->params);
                if (!params) {
                    params = json_object_new_string(msg->params);
                }
            } else {
                params = json_object_new_object();
            }
            
            json_object_object_add(json_msg, "id", id);
            json_object_object_add(json_msg, "method", method);
            json_object_object_add(json_msg, "params", params);
            break;
        }
        case MCP_RESPONSE: {
            json_object *id = json_object_new_int(msg->id);
            json_object *result;
            
            if (msg->params) {
                result = json_tokener_parse(msg->params);
                if (!result) {
                    result = json_object_new_string(msg->params);
                }
            } else {
                result = json_object_new_object();
            }
            
            json_object_object_add(json_msg, "id", id);
            json_object_object_add(json_msg, "result", result);
            break;
        }
        case MCP_NOTIFICATION: {
            json_object *method = json_object_new_string(msg->method ? msg->method : "");
            json_object *params;
            
            if (msg->params) {
                params = json_tokener_parse(msg->params);
                if (!params) {
                    params = json_object_new_string(msg->params);
                }
            } else {
                params = json_object_new_object();
            }
            
            json_object_object_add(json_msg, "method", method);
            json_object_object_add(json_msg, "params", params);
            break;
        }
    }
    
    const char *json_str = json_object_to_json_string(json_msg);
    strncpy(buffer, json_str, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    
    json_object_put(json_msg);
    return 0;
}
