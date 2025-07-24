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
#include "../../rknn/call_rknn_init/call_rknn_init.h"
#include "../../rknn/call_rknn_query/call_rknn_query.h"
#include "../../rknn/call_rknn_destroy/call_rknn_destroy.h"
#include "../../rknn/call_rknn_run/call_rknn_run.h"
#include "../../rknn/get_rknn_constants/get_rknn_constants.h"
#include "../../rknn/call_rknn_inputs_set/call_rknn_inputs_set.h"
#include "../../rknn/call_rknn_outputs_get/call_rknn_outputs_get.h"
#include "../../rknn/call_rknn_outputs_release/call_rknn_outputs_release.h"
#include "../../rknn/call_rknn_wait/call_rknn_wait.h"
#include "../../rknn/call_rknn_set_input_shapes/call_rknn_set_input_shapes.h"
#include "../../rknn/call_rknn_set_input_shape/call_rknn_set_input_shape.h"
#include "../../rknn/call_rknn_create_mem/call_rknn_create_mem.h"
#include "../../rknn/call_rknn_create_mem2/call_rknn_create_mem2.h"
#include "../../rknn/call_rknn_destroy_mem/call_rknn_destroy_mem.h"
#include "../../rknn/call_rknn_set_weight_mem/call_rknn_set_weight_mem.h"
#include "../../rknn/call_rknn_set_internal_mem/call_rknn_set_internal_mem.h"
#include "../../rknn/call_rknn_set_io_mem/call_rknn_set_io_mem.h"
#include "../../rknn/call_rknn_mem_sync/call_rknn_mem_sync.h"
#include "../../rknn/call_rknn_dup_context/call_rknn_dup_context.h"
#include "../../rknn/call_rknn_set_core_mask/call_rknn_set_core_mask.h"
#include "../../rknn/call_rknn_set_batch_core_num/call_rknn_set_batch_core_num.h"
#include "../../utils/log_message/log_message.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int handle_request(JSONRPCRequest* req, Connection* conn) {
    if (!req || !conn || !req->is_valid) {
        return -1;
    }
    
    // Direct method dispatch - no unified parameter handling needed
    // Each function handles its own parameter format as designed
    json_object* result = NULL;
    
    // Handle different methods - each function manages its own parameters
    if (strcmp(req->method, "rkllm.createDefaultParam") == 0) {
        result = call_rkllm_createDefaultParam();
    } else if (strcmp(req->method, "rkllm.init") == 0) {
        result = call_rkllm_init(req->params);
    } else if (strcmp(req->method, "rkllm.run") == 0) {
        int request_id = req->id && json_object_is_type(req->id, json_type_int) ? 
                        json_object_get_int(req->id) : 0;
        result = call_rkllm_run(req->params, conn->fd, request_id);
        
        if (!result) {
            LOG_DEBUG_MSG("rkllm.run returned NULL - async streaming active");
            return 0; // Success - callback handles responses
        }
    } else if (strcmp(req->method, "rkllm.run_async") == 0) {
        int request_id = req->id && json_object_is_type(req->id, json_type_int) ? 
                        json_object_get_int(req->id) : 0;
        LOG_DEBUG_MSG("Calling rkllm.run_async");
        result = call_rkllm_run_async(req->params, conn->fd, request_id);
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
        
    // RKNN methods - Core functions
    } else if (strcmp(req->method, "rknn.init") == 0) {
        result = call_rknn_init(req->params);
    } else if (strcmp(req->method, "rknn.query") == 0) {
        result = call_rknn_query(req->params);
    } else if (strcmp(req->method, "rknn.run") == 0) {
        result = call_rknn_run(req->params);
    } else if (strcmp(req->method, "rknn.wait") == 0) {
        result = call_rknn_wait(req->params);
    } else if (strcmp(req->method, "rknn.destroy") == 0) {
        result = call_rknn_destroy(req->params);
    } else if (strcmp(req->method, "rknn.dup_context") == 0) {
        result = call_rknn_dup_context(req->params);
    } else if (strcmp(req->method, "rknn.get_constants") == 0) {
        result = get_rknn_constants();
        
    // RKNN methods - Input/Output functions  
    } else if (strcmp(req->method, "rknn.inputs_set") == 0) {
        result = call_rknn_inputs_set(req->params);
    } else if (strcmp(req->method, "rknn.outputs_get") == 0) {
        result = call_rknn_outputs_get(req->params);
    } else if (strcmp(req->method, "rknn.outputs_release") == 0) {
        result = call_rknn_outputs_release(req->params);
    } else if (strcmp(req->method, "rknn.set_input_shapes") == 0) {
        result = call_rknn_set_input_shapes(req->params);
    } else if (strcmp(req->method, "rknn.set_input_shape") == 0) {
        result = call_rknn_set_input_shape(req->params);
        
    // RKNN methods - Memory management functions (only available ones)
    } else if (strcmp(req->method, "rknn.create_mem") == 0) {
        result = call_rknn_create_mem(req->params);
    } else if (strcmp(req->method, "rknn.create_mem2") == 0) {
        result = call_rknn_create_mem2(req->params);
    } else if (strcmp(req->method, "rknn.destroy_mem") == 0) {
        result = call_rknn_destroy_mem(req->params);
    } else if (strcmp(req->method, "rknn.set_weight_mem") == 0) {
        result = call_rknn_set_weight_mem(req->params);
    } else if (strcmp(req->method, "rknn.set_internal_mem") == 0) {
        result = call_rknn_set_internal_mem(req->params);
    } else if (strcmp(req->method, "rknn.set_io_mem") == 0) {
        result = call_rknn_set_io_mem(req->params);
    } else if (strcmp(req->method, "rknn.mem_sync") == 0) {
        result = call_rknn_mem_sync(req->params);
        
    // RKNN methods - Configuration functions
    } else if (strcmp(req->method, "rknn.set_core_mask") == 0) {
        result = call_rknn_set_core_mask(req->params);
    } else if (strcmp(req->method, "rknn.set_batch_core_num") == 0) {
        result = call_rknn_set_batch_core_num(req->params);
    } else {
        return send_error_response(conn, req->id, -32601, "Method not found");
    }
    
    // Send response
    if (!result) {
        return send_error_response(conn, req->id, -32000, "Internal server error");
    }
    
    char* response_str = format_response(req->id, result);
    json_object_put(result);
    
    if (!response_str) {
        return -1;
    }
    
    int send_result = send_to_connection(conn, response_str, strlen(response_str));
    free(response_str);
    
    return send_result > 0 ? 0 : -1;
}

// Helper function to send error responses
int send_error_response(Connection* conn, json_object* id, int code, const char* message) {
    json_object* error = json_object_new_object();
    json_object_object_add(error, "code", json_object_new_int(code));
    json_object_object_add(error, "message", json_object_new_string(message));
    
    json_object* response = json_object_new_object();
    json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
    if (id) {
        json_object_object_add(response, "id", json_object_get(id));
    } else {
        json_object_object_add(response, "id", NULL);
    }
    json_object_object_add(response, "error", error);
    
    const char* error_str = json_object_to_json_string(response);
    int send_result = send_to_connection(conn, error_str, strlen(error_str));
    json_object_put(response);
    return send_result > 0 ? 0 : -1;
}