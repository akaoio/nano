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
 * REFACTORED IO ARCHITECTURE - PURE AND CLEAN
 * 
 * This IO system has been refactored to support streaming and remove complexity:
 * 
 * OLD ARCHITECTURE (REMOVED):
 * - Dual queue system (request + response queues)
 * - NANO polling for responses with timeouts
 * - Buffer accumulation in RKLLM callbacks
 * - Test/mock code contamination
 *
 * NEW ARCHITECTURE (CURRENT):
 * - Single request queue only
 * - Direct NANO callback mechanism (no response queue)
 * - RKLLM callbacks stream chunks directly to NANO 
 * - Pure processing with proper parameter validation
 * - Support for both synchronous and streaming operations
 * 
 * FLOW:
 * Client → NANO → IO request queue → IO worker → RKLLM → 
 * → RKLLM callback → Direct NANO callback → Client
 *
 * STREAMING FLOW:
 * RKLLM callback → io_streaming_chunk_callback → NANO callback → Client (real-time)
 */

// Global IO context
io_context_t g_io_context;

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
        if (pthread_create(&g_io_context.workers[i], nullptr, io_worker_thread, nullptr) == 0) {
            atomic_fetch_add(&g_io_context.active_workers, 1);
        }
    }
    
    return IO_OK;
}

int io_push_request(const char* json_request) {
    printf("\n🔧 === IO PUSH REQUEST ===\n");
    printf("📥 Received request: %s\n", json_request ? json_request : "NULL");
    
    if (!atomic_load(&g_io_context.running)) {
        printf("❌ IO context not running\n");
        return IO_ERROR;
    }
    
    if (!json_request) {
        printf("❌ JSON request is NULL\n");
        return IO_ERROR;
    }
    
    // Parse JSON request
    uint32_t request_id, handle_id;
    char method[32], params[4096];
    
    if (io_parse_json_request(json_request, &request_id, &handle_id, 
                             method, params) != IO_OK) {
        printf("❌ Failed to parse JSON request\n");
        return IO_ERROR;
    }
    
    printf("✅ Parsed: ID=%d, Method=%s, Params=%s\n", request_id, method, params);
    
    // Create queue item using designated initializers
    queue_item_t item = {
        .handle_id = handle_id,
        .request_id = request_id,
        .params = strdup(params),
        .params_len = strlen(params),
        .timestamp = (uint64_t)time(nullptr)
    };
    snprintf(item.method, sizeof(item.method), "%s", method);
    
    if (queue_push(&g_io_context.request_queue, &item) != 0) {
        printf("❌ Failed to push to queue\n");
        free(item.params);
        return IO_QUEUE_FULL;
    }
    
    printf("✅ Request pushed to queue successfully\n");
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
