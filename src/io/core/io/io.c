#define _GNU_SOURCE
#include "io.h"
#include "../../operations.h"
#include "../../../common/core.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/*
 * IO System - Pure callback-based request processing with intelligent queuing
 * 
 * QUEUE STRATEGY:
 * - Queue: run, run_async, run_streaming (inference operations)
 * - Direct: abort, is_running, init, destroy, cache operations
 */

// Global IO context
io_context_t g_io_context;

// Check if operation needs queuing or can be processed directly
static bool operation_needs_queue(const char* method) {
    if (!method) return true; // Default to queue for safety
    
    // Operations that MUST be queued (inference operations)
    if (strcmp(method, "run") == 0 ||
        strcmp(method, "run_async") == 0 ||
        strcmp(method, "run_streaming") == 0 ||
        strcmp(method, "run_async_streaming") == 0) {
        return true;
    }
    
    // Operations that can be processed directly (non-inference)
    if (strcmp(method, "abort") == 0 ||
        strcmp(method, "is_running") == 0 ||
        strcmp(method, "init") == 0 ||
        strcmp(method, "destroy") == 0 ||
        strcmp(method, "clear_kv_cache") == 0 ||
        strcmp(method, "get_kv_cache_size") == 0 ||
        strcmp(method, "load_lora") == 0 ||
        strcmp(method, "load_prompt_cache") == 0 ||
        strcmp(method, "release_prompt_cache") == 0 ||
        strcmp(method, "set_chat_template") == 0 ||
        strcmp(method, "set_function_tools") == 0 ||
        strcmp(method, "set_cross_attn_params") == 0 ||
        strcmp(method, "create_default_param") == 0) {
        return false; // Process directly
    }
    
    // Unknown operations default to queue for safety
    return true;
}

// Process operation directly (bypass queue)
static int io_process_direct(const char* method, const char* params, char* response, size_t response_size) {
    // Call io_process_request directly for immediate processing
    char json_request[8192];
    snprintf(json_request, sizeof(json_request),
             "{ \"jsonrpc\": \"2.0\", \"id\": 999, \"method\": \"%s\", \"params\": %s }",
             method, params ? params : "{}");
    
    return io_process_request(json_request, response, response_size);
}

int io_init(nano_callback_t callback, void* userdata) {
    if (atomic_load(&g_io_context.running)) return IO_OK;
    
    // Initialize request queue only
    if (queue_init(&g_io_context.request_queue) != 0) return IO_ERROR;
    
    // Set callback to NANO
    g_io_context.nano_callback = callback;
    g_io_context.nano_userdata = userdata;
    
    // Initialize IO operations
    if (io_operations_init() != 0) return IO_ERROR;
    
    atomic_store(&g_io_context.running, 1);
    atomic_store(&g_io_context.active_workers, 0);
    
    // Start worker threads
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pthread_create(&g_io_context.workers[i], NULL, io_worker_thread, NULL) == 0) {
            atomic_fetch_add(&g_io_context.active_workers, 1);
        }
    }
    
    return IO_OK;
}

int io_push_request(const char* json_request) {
    if (!atomic_load(&g_io_context.running) || !json_request) return IO_ERROR;
    
    // Parse JSON request
    uint32_t request_id, handle_id;
    char method[32], params[4096];
    
    if (io_parse_json_request_with_handle(json_request, &request_id, &handle_id, 
                                         method, params) != IO_OK) {
        return IO_ERROR;
    }
    
    // Check if operation can be processed directly (bypass queue)
    if (!operation_needs_queue(method)) {
        char response[8192];
        int result = io_process_direct(method, params, response, sizeof(response));
        
        // Send response directly via callback
        if (g_io_context.nano_callback && result == IO_OK) {
            g_io_context.nano_callback(response, g_io_context.nano_userdata);
        }
        
        return result;
    }
    
    // Create queue item using designated initializers
    queue_item_t item = {
        .handle_id = handle_id,
        .request_id = request_id,
        .params = strdup(params),
        .params_len = strlen(params),
        .timestamp = (uint64_t)time(NULL)
    };
    snprintf(item.method, sizeof(item.method), "%s", method);
    
    if (queue_push(&g_io_context.request_queue, &item) != 0) {
        free(item.params);
        return IO_QUEUE_FULL;
    }
    
    return IO_OK;
}

void io_set_streaming_callback(nano_callback_t callback, void* userdata) {
    g_io_context.nano_callback = callback;
    g_io_context.nano_userdata = userdata;
}

void io_shutdown(void) {
    if (!atomic_load(&g_io_context.running)) return;
    
    atomic_store(&g_io_context.running, 0);
    
    // Wait for workers to finish with timeout
    int timeout = 3000; // 3 second timeout
    while (atomic_load(&g_io_context.active_workers) > 0 && timeout-- > 0) {
        usleep(1000);
    }
    
    if (atomic_load(&g_io_context.active_workers) > 0) {
        printf("⚠️  Warning: %d workers still active after shutdown timeout\n", 
               atomic_load(&g_io_context.active_workers));
    }
    
    // Clean up request queue
    queue_item_t item;
    while (queue_pop(&g_io_context.request_queue, &item) == 0) {
        queue_item_cleanup(&item);
    }
    
    // Shutdown operations
    io_operations_shutdown();
}
