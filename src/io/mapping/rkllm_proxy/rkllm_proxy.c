#include "rkllm_proxy.h"
#include "rkllm_operations.h"
#include "../handle_pool/handle_pool.h"
#include "../../../common/memory_utils/memory_utils.h"
#include "../../../common/string_utils/string_utils.h"
#include "../../../libs/rkllm/rkllm.h"
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// External global handle pool
extern handle_pool_t g_handle_pool;
static bool g_initialized = false;

int rkllm_proxy_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    if (handle_pool_init(&g_handle_pool) != 0) {
        return -1;
    }
    
    g_initialized = true;
    return 0;
}

void rkllm_proxy_shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    // Properly destroy all RKLLM handles to free NPU memory
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (g_handle_pool.slots[i].active) {
            LLMHandle handle = g_handle_pool.slots[i].handle;
            if (handle) {
                rkllm_destroy(handle);
            }
            (void)handle_pool_destroy(&g_handle_pool, g_handle_pool.slots[i].id);
        }
    }
    
    g_initialized = false;
}

const char* rkllm_proxy_get_operation_name(rkllm_operation_t op) {
    if (op >= OP_MAX) {
        return nullptr;
    }
    return OPERATION_NAMES[op];
}

rkllm_operation_t rkllm_proxy_get_operation_by_name(const char* name) {
    if (!name) {
        return OP_MAX;
    }
    
    for (int i = 0; i < OP_MAX; i++) {
        if (strcmp(name, OPERATION_NAMES[i]) == 0) {
            return (rkllm_operation_t)i;
        }
    }
    return OP_MAX;
}

int rkllm_proxy_execute(const rkllm_request_t* request, rkllm_result_t* result) {
    if (!g_initialized) {
        return -1;
    }
    
    if (!request || !result) {
        return -1;
    }
    
    // Initialize result
    memset(result, 0, sizeof(rkllm_result_t));
    result->handle_id = request->handle_id;
    result->status = -1;
    
    // Special case for init operation
    if (request->operation == OP_INIT) {
        uint32_t new_handle_id = 0;
        result->status = INIT_HANDLER(&new_handle_id, request->params_json, result);
        result->handle_id = new_handle_id;
        return result->status;
    }
    
    // Validate operation
    if (request->operation >= OP_MAX) {
        return -1;
    }
    
    // Get handler
    rkllm_op_handler_t handler = OPERATION_HANDLERS[request->operation];
    if (!handler) {
        return -1;
    }
    
    // Execute operation
    result->status = handler(request->handle_id, request->params_json, result);
    return result->status;
}

// Forward declaration for streaming callback  
extern void io_streaming_chunk_callback(const char* chunk, bool is_final, void* userdata);

int rkllm_proxy_execute_streaming(const rkllm_request_t* request, rkllm_result_t* result, uint32_t request_id) {
    if (!g_initialized) {
        return -1;
    }
    
    if (!request || !result) {
        return -1;
    }
    
    // Initialize result
    memset(result, 0, sizeof(rkllm_result_t));
    result->handle_id = request->handle_id;
    result->status = -1;
    
    // Special case for init operation - no streaming for init
    if (request->operation == OP_INIT) {
        uint32_t new_handle_id = 0;
        result->status = INIT_HANDLER(&new_handle_id, request->params_json, result);
        result->handle_id = new_handle_id;
        return result->status;
    }
    
    // Validate operation
    if (request->operation >= OP_MAX) {
        return -1;
    }
    
    // Only support streaming for RUN and RUN_ASYNC operations
    if (request->operation != OP_RUN && request->operation != OP_RUN_ASYNC) {
        // For non-streaming operations, fall back to normal execution
        rkllm_op_handler_t handler = OPERATION_HANDLERS[request->operation];
        if (!handler) {
            return -1;
        }
        result->status = handler(request->handle_id, request->params_json, result);
        return result->status;
    }
    
    // Execute streaming version of RUN operation
    if (request->operation == OP_RUN) {
        result->status = rkllm_op_run_streaming(request->handle_id, request->params_json, result, request_id);
    } else if (request->operation == OP_RUN_ASYNC) {
        result->status = rkllm_op_run_async_streaming(request->handle_id, request->params_json, result, request_id);
    }
    
    return result->status;
}

void rkllm_proxy_free_result(rkllm_result_t* result) {
    if (!result) {
        return;
    }
    
    if (result->result_data) {
        free(result->result_data);
        result->result_data = nullptr;
    }
    result->result_size = 0;
}

// Helper function to get handle from pool
LLMHandle rkllm_proxy_get_handle(uint32_t handle_id) {
    if (!g_initialized) {
        return nullptr;
    }
    
    LLMHandle* handle_ptr = handle_pool_get(&g_handle_pool, handle_id);
    return handle_ptr ? *handle_ptr : nullptr;
}

// Helper function to create JSON result using json-c
char* rkllm_proxy_create_json_result(int status, const char* data) {
    json_object *result = json_object_new_object();
    json_object *status_obj = json_object_new_int(status);
    
    json_object_object_add(result, "status", status_obj);
    
    if (data) {
        json_object *data_obj = json_tokener_parse(data);
        if (!data_obj) {
            data_obj = json_object_new_string(data);
        }
        json_object_object_add(result, "data", data_obj);
    }
    
    const char *json_str = json_object_to_json_string(result);
    char *result_str = strdup(json_str);
    
    json_object_put(result);
    return result_str;
}

// Helper function to create JSON error result using json-c
char* rkllm_proxy_create_error_result(int status, const char* error_msg) {
    json_object *result = json_object_new_object();
    json_object *status_obj = json_object_new_int(status);
    json_object *error_obj = json_object_new_string(error_msg ? error_msg : "Unknown error");
    
    json_object_object_add(result, "status", status_obj);
    json_object_object_add(result, "error", error_obj);
    
    const char *json_str = json_object_to_json_string(result);
    char *result_str = strdup(json_str);
    
    json_object_put(result);
    return result_str;
}

// Global callback function for RKLLM results
int rkllm_proxy_global_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (!result || !userdata) {
        return 0;
    }
    
    rkllm_callback_context_t* context = (rkllm_callback_context_t*)userdata;
    
    // Handle streaming mode
    if (context->streaming_enabled && context->stream_callback) {
        // Stream chunks directly via callback
        if (result->text && strlen(result->text) > 0) {
            bool is_final = (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR);
            context->stream_callback(result->text, is_final, context->stream_userdata);
        }
        
        // Send final signal if needed
        if (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR) {
            if (!result->text || strlen(result->text) == 0) {
                // Send empty final chunk if no text in final callback
                context->stream_callback("", true, context->stream_userdata);
            }
            context->final_status = (state == RKLLM_RUN_FINISH) ? 0 : -1;
        }
    } else {
        // Legacy mode: accumulate in buffer
        if (result->text && context->output_buffer) {
            size_t text_len = strlen(result->text);
            size_t remaining = context->buffer_size - context->current_pos - 1;
            
            if (text_len < remaining) {
                strcpy(context->output_buffer + context->current_pos, result->text);
                context->current_pos += text_len;
            }
        }
        
        if (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR) {
            context->final_status = (state == RKLLM_RUN_FINISH) ? 0 : -1;
        }
    }
    
    // Update context state
    context->call_state = state;
    
    return 0; // Continue processing
}

// Create callback context
rkllm_callback_context_t* rkllm_proxy_create_callback_context(size_t buffer_size) {
    rkllm_callback_context_t* context = malloc(sizeof(rkllm_callback_context_t));
    if (!context) {
        return nullptr;
    }
    
    context->output_buffer = malloc(buffer_size);
    if (!context->output_buffer) {
        free(context);
        return nullptr;
    }
    
    context->buffer_size = buffer_size;
    context->current_pos = 0;
    context->final_status = 0;
    context->call_state = RKLLM_RUN_NORMAL;
    context->output_buffer[0] = '\0';
    
    // Initialize streaming fields
    context->request_id = 0;
    context->streaming_enabled = false;
    context->stream_callback = nullptr;
    context->stream_userdata = nullptr;
    
    return context;
}

// Destroy callback context
void rkllm_proxy_destroy_callback_context(rkllm_callback_context_t* context) {
    if (context) {
        if (context->output_buffer) {
            free(context->output_buffer);
        }
        free(context);
    }
}
