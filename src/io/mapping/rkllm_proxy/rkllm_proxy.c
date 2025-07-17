#include "rkllm_proxy.h"
#include "rkllm_operations.h"
#include "../handle_pool/handle_pool.h"
#include "../../../common/memory_utils/memory_utils.h"
#include "../../../common/string_utils/string_utils.h"
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
    
    // Cleanup all handles
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (g_handle_pool.slots[i].active) {
            handle_pool_destroy(&g_handle_pool, g_handle_pool.slots[i].id);
        }
    }
    
    g_initialized = false;
}

const char* rkllm_proxy_get_operation_name(rkllm_operation_t op) {
    if (op >= OP_MAX) {
        return NULL;
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

void rkllm_proxy_free_result(rkllm_result_t* result) {
    if (!result) {
        return;
    }
    
    if (result->result_data) {
        free(result->result_data);
        result->result_data = NULL;
    }
    result->result_size = 0;
}

// Helper function to get handle from pool
LLMHandle rkllm_proxy_get_handle(uint32_t handle_id) {
    if (!g_initialized) {
        return NULL;
    }
    
    LLMHandle* handle_ptr = handle_pool_get(&g_handle_pool, handle_id);
    return handle_ptr ? *handle_ptr : NULL;
}

// Helper function to create JSON result
char* rkllm_proxy_create_json_result(int status, const char* data) {
    static char result_buffer[4096];
    
    if (data) {
        snprintf(result_buffer, sizeof(result_buffer), 
                "{\"status\":%d,\"data\":%s}", status, data);
    } else {
        snprintf(result_buffer, sizeof(result_buffer), 
                "{\"status\":%d}", status);
    }
    
    return strdup(result_buffer);
}

// Helper function to create JSON error result
char* rkllm_proxy_create_error_result(int status, const char* error_msg) {
    static char result_buffer[4096];
    
    snprintf(result_buffer, sizeof(result_buffer), 
            "{\"status\":%d,\"error\":\"%s\"}", status, error_msg ? error_msg : "Unknown error");
    
    return strdup(result_buffer);
}

// Global callback function for RKLLM results
int rkllm_proxy_global_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (!result || !userdata) {
        return 0;
    }
    
    rkllm_callback_context_t* context = (rkllm_callback_context_t*)userdata;
    
    // Append text to output buffer if available
    if (result->text && context->output_buffer) {
        size_t text_len = strlen(result->text);
        size_t remaining = context->buffer_size - context->current_pos - 1;
        
        if (text_len < remaining) {
            strcpy(context->output_buffer + context->current_pos, result->text);
            context->current_pos += text_len;
        }
    }
    
    // Update context state
    context->call_state = state;
    
    if (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR) {
        context->final_status = (state == RKLLM_RUN_FINISH) ? 0 : -1;
    }
    
    return 0; // Continue processing
}

// Create callback context
rkllm_callback_context_t* rkllm_proxy_create_callback_context(size_t buffer_size) {
    rkllm_callback_context_t* context = malloc(sizeof(rkllm_callback_context_t));
    if (!context) {
        return NULL;
    }
    
    context->output_buffer = malloc(buffer_size);
    if (!context->output_buffer) {
        free(context);
        return NULL;
    }
    
    context->buffer_size = buffer_size;
    context->current_pos = 0;
    context->final_status = 0;
    context->call_state = RKLLM_RUN_NORMAL;
    context->output_buffer[0] = '\0';
    
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
