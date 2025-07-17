#define _GNU_SOURCE
#include "io.h"
#include "../../operations.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Global IO context
io_context_t g_io_context;

int io_init(void) {
    if (atomic_load(&g_io_context.running)) return IO_OK;
    
    // Initialize queues
    if (queue_init(&g_io_context.request_queue) != 0) return IO_ERROR;
    if (queue_init(&g_io_context.response_queue) != 0) return IO_ERROR;
    
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
    
    if (io_parse_json_request(json_request, &request_id, &handle_id, 
                             method, params) != IO_OK) {
        return IO_ERROR;
    }
    
    // Create queue item
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

int io_pop_response(char* json_response, size_t max_len) {
    if (!atomic_load(&g_io_context.running) || !json_response) return IO_ERROR;
    
    queue_item_t item;
    if (queue_pop(&g_io_context.response_queue, &item) != 0) {
        return IO_TIMEOUT; // No response available
    }
    
    if (item.params && strlen(item.params) < max_len) {
        strncpy(json_response, item.params, max_len - 1);
        json_response[max_len - 1] = '\0';
        queue_item_cleanup(&item);
        return IO_OK;
    }
    
    queue_item_cleanup(&item);
    return IO_ERROR;
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
    
    // Clean up queues
    queue_item_t item;
    while (queue_pop(&g_io_context.request_queue, &item) == 0) {
        queue_item_cleanup(&item);
    }
    while (queue_pop(&g_io_context.response_queue, &item) == 0) {
        queue_item_cleanup(&item);
    }
    
    // Shutdown operations
    io_operations_shutdown();
}
