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
extern io_context_t g_io_context;

void* io_worker_thread(void* arg) {
    (void)arg;
    
    while (atomic_load(&g_io_context.running)) {
        queue_item_t item;
        
        // Try to get a request
        if (queue_pop(&g_io_context.request_queue, &item) != 0) {
            usleep(1000); // 1ms sleep when no work
            continue;
        }
        
        // Check for timeout
        uint64_t now = (uint64_t)time(NULL);
        if (now - item.timestamp > REQUEST_TIMEOUT_MS / 1000) {
            // Create timeout response
            char response[1024];
            snprintf(response, sizeof(response), 
                    "{\"jsonrpc\":\"2.0\",\"id\":%u,\"error\":{\"code\":%d,\"message\":\"Request timeout\"}}", 
                    item.request_id, IO_TIMEOUT);
            
            queue_item_t resp_item = {
                .handle_id = item.handle_id,
                .request_id = item.request_id,
                .params = strdup(response),
                .params_len = strlen(response),
                .timestamp = now
            };
            strncpy(resp_item.method, "response", sizeof(resp_item.method) - 1);
            
            queue_push(&g_io_context.response_queue, &resp_item);
            queue_item_cleanup(&item);
            continue;
        }
        
        // Process the request
        // Reconstruct full JSON-RPC request
        char json_request[8192];
        snprintf(json_request, sizeof(json_request),
                "{\"jsonrpc\":\"2.0\",\"id\":%u,\"method\":\"%s\",\"params\":%s}",
                item.request_id, item.method, item.params);
        
        // Process request
        char response[16384];
        (void)io_process_request(json_request, response, sizeof(response));
        
        // Push to response queue
        queue_item_t resp_item = {
            .handle_id = item.handle_id,
            .request_id = item.request_id,
            .params = strdup(response),
            .params_len = strlen(response),
            .timestamp = (uint64_t)time(NULL)
        };
        strncpy(resp_item.method, "response", sizeof(resp_item.method) - 1);
        
        while (queue_push(&g_io_context.response_queue, &resp_item) != 0) {
            usleep(1000); // Wait if response queue is full
        }
        
        queue_item_cleanup(&item);
    }
    
    atomic_fetch_sub(&g_io_context.active_workers, 1);
    return NULL;
}
