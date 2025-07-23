#include "handle_request.h"
#include "../format_response/format_response.h"
#include "../../connection/send_to_connection/send_to_connection.h"
#include "../../rkllm/call_rkllm_createDefaultParam/call_rkllm_createDefaultParam.h"
#include "../../rkllm/call_rkllm_init/call_rkllm_init.h"
#include "../../rkllm/call_rkllm_run/call_rkllm_run.h"
#include "../../rkllm/call_rkllm_run_async/call_rkllm_run_async.h"
#include "../../rkllm/call_rkllm_is_running/call_rkllm_is_running.h"
#include "../../rkllm/call_rkllm_abort/call_rkllm_abort.h"
#include "../../rkllm/call_rkllm_destroy/call_rkllm_destroy.h"
#include "../../rkllm/call_rkllm_load_lora/call_rkllm_load_lora.h"
#include "../../rkllm/call_rkllm_load_prompt_cache/call_rkllm_load_prompt_cache.h"
#include "../../rkllm/call_rkllm_release_prompt_cache/call_rkllm_release_prompt_cache.h"
#include "../../rkllm/call_rkllm_clear_kv_cache/call_rkllm_clear_kv_cache.h"
#include "../../rkllm/call_rkllm_get_kv_cache_size/call_rkllm_get_kv_cache_size.h"
#include "../../rkllm/call_rkllm_set_chat_template/call_rkllm_set_chat_template.h"
#include "../../rkllm/call_rkllm_set_function_tools/call_rkllm_set_function_tools.h"
#include "../../rkllm/call_rkllm_set_cross_attn_params/call_rkllm_set_cross_attn_params.h"
#include "../../rkllm/get_rkllm_constants/get_rkllm_constants.h"
#include "../../utils/log_message/log_message.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int handle_request(JSONRPCRequest* req, Connection* conn) {
    if (!req || !conn || !req->is_valid) {
        return -1;
    }
    
    json_object* result = NULL;
    
    // Handle different methods - following DESIGN.md 1:1 RKLLM mapping
    if (strcmp(req->method, "rkllm.createDefaultParam") == 0) {
        result = call_rkllm_createDefaultParam();
    } else if (strcmp(req->method, "rkllm.init") == 0) {
        result = call_rkllm_init(req->params);
    } else if (strcmp(req->method, "rkllm.run") == 0) {
        // Extract request ID for streaming context
        int request_id = 0;
        if (req->id && json_object_is_type(req->id, json_type_int)) {
            request_id = json_object_get_int(req->id);
        }
        result = call_rkllm_run(req->params, conn->fd, request_id);
        
        // CRITICAL FIX: For rkllm.run, NULL return means "async streaming in progress"
        // The callback handles all responses, so don't send error - just return success
        if (!result) {
            LOG_DEBUG_MSG("rkllm.run returned NULL - async streaming active, no response needed");
            return 0; // Success - callback handles responses
        }
    } else if (strcmp(req->method, "rkllm.run_async") == 0) {
        // Extract request ID for callback context
        int request_id = 0;
        if (req->id && json_object_is_type(req->id, json_type_int)) {
            request_id = json_object_get_int(req->id);
        }
        LOG_DEBUG_MSG("Calling rkllm.run_async with client_fd=%d, request_id=%d", conn->fd, request_id);
        result = call_rkllm_run_async(req->params, conn->fd, request_id);
        LOG_DEBUG_MSG("call_rkllm_run_async returned: %p", (void*)result);
    } else if (strcmp(req->method, "rkllm.is_running") == 0) {
        result = call_rkllm_is_running();
    } else if (strcmp(req->method, "rkllm.abort") == 0) {
        result = call_rkllm_abort();
    } else if (strcmp(req->method, "rkllm.destroy") == 0) {
        result = call_rkllm_destroy();
    } else if (strcmp(req->method, "rkllm.load_lora") == 0) {
        result = call_rkllm_load_lora(req->params);
    } else if (strcmp(req->method, "rkllm.load_prompt_cache") == 0) {
        result = call_rkllm_load_prompt_cache(req->params);
    } else if (strcmp(req->method, "rkllm.release_prompt_cache") == 0) {
        result = call_rkllm_release_prompt_cache();
    } else if (strcmp(req->method, "rkllm.clear_kv_cache") == 0) {
        result = call_rkllm_clear_kv_cache(req->params);
    } else if (strcmp(req->method, "rkllm.get_kv_cache_size") == 0) {
        result = call_rkllm_get_kv_cache_size();
    } else if (strcmp(req->method, "rkllm.set_chat_template") == 0) {
        result = call_rkllm_set_chat_template(req->params);
    } else if (strcmp(req->method, "rkllm.set_function_tools") == 0) {
        result = call_rkllm_set_function_tools(req->params);
    } else if (strcmp(req->method, "rkllm.set_cross_attn_params") == 0) {
        result = call_rkllm_set_cross_attn_params(req->params);
    } else if (strcmp(req->method, "rkllm.get_constants") == 0) {
        result = get_rkllm_constants();
    } else {
        // Method not found - return error
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32601));
        json_object_object_add(error, "message", json_object_new_string("Method not found"));
        
        // Format error response
        json_object* response = json_object_new_object();
        json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
        if (req->id) {
            json_object_object_add(response, "id", json_object_get(req->id));
        } else {
            json_object_object_add(response, "id", NULL);
        }
        json_object_object_add(response, "error", error);
        
        const char* error_str = json_object_to_json_string(response);
        int send_result = send_to_connection(conn, error_str, strlen(error_str));
        json_object_put(response);
        return send_result > 0 ? 0 : -1;
    }
    
    if (!result) {
        // Function returned NULL - create error response
        json_object* error = json_object_new_object();
        json_object_object_add(error, "code", json_object_new_int(-32000));
        json_object_object_add(error, "message", json_object_new_string("Internal server error"));
        
        // Format error response
        json_object* response = json_object_new_object();
        json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
        if (req->id) {
            json_object_object_add(response, "id", json_object_get(req->id));
        } else {
            json_object_object_add(response, "id", NULL);
        }
        json_object_object_add(response, "error", error);
        
        const char* error_str = json_object_to_json_string(response);
        int send_result = send_to_connection(conn, error_str, strlen(error_str));
        json_object_put(response);
        return send_result > 0 ? 0 : -1;
    }
    
    // Format successful response
    char* response_str = format_response(req->id, result);
    json_object_put(result);
    
    if (!response_str) {
        return -1;
    }
    
    int send_result = send_to_connection(conn, response_str, strlen(response_str));
    free(response_str);
    
    return send_result > 0 ? 0 : -1;
}