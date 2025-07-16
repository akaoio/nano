#define _GNU_SOURCE
#include "io.h"
#include "queue.h"
#include "handle_pool.h"
#include "operations.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

// Global context
static struct {
    queue_t request_queue;
    queue_t response_queue;
    pthread_t workers[MAX_WORKERS];
    _Atomic int running;
    _Atomic int active_workers;
} g_io;

// Global handle pool
handle_pool_t g_pool;

// Worker thread function
static void* worker_thread(void* arg) {
    (void)arg;
    
    while (atomic_load(&g_io.running)) {
        queue_item_t item;
        
        // Try to get a request
        if (queue_pop(&g_io.request_queue, &item) != 0) {
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
            
            queue_push(&g_io.response_queue, &resp_item);
            queue_item_cleanup(&item);
            continue;
        }
        
        // Process the request
        char result[8192];
        int ret = execute_method(item.method, item.handle_id, item.params, result, sizeof(result));
        
        // Create response
        char response[16384];
        if (ret == 0) {
            snprintf(response, sizeof(response), 
                    "{\"jsonrpc\":\"2.0\",\"id\":%u,\"result\":%s}", 
                    item.request_id, result);
        } else {
            snprintf(response, sizeof(response), 
                    "{\"jsonrpc\":\"2.0\",\"id\":%u,\"error\":{\"code\":%d,\"message\":\"%s\"}}", 
                    item.request_id, ret, "Method execution failed");
        }
        
        // Push to response queue
        queue_item_t resp_item = {
            .handle_id = item.handle_id,
            .request_id = item.request_id,
            .params = strdup(response),
            .params_len = strlen(response),
            .timestamp = (uint64_t)time(NULL)
        };
        strncpy(resp_item.method, "response", sizeof(resp_item.method) - 1);
        
        while (queue_push(&g_io.response_queue, &resp_item) != 0) {
            usleep(1000); // Wait if response queue is full
        }
        
        queue_item_cleanup(&item);
    }
    
    atomic_fetch_sub(&g_io.active_workers, 1);
    return NULL;
}

int io_init(void) {
    if (atomic_load(&g_io.running)) return IO_OK;
    
    // Initialize queues
    if (queue_init(&g_io.request_queue) != 0) return IO_ERROR;
    if (queue_init(&g_io.response_queue) != 0) return IO_ERROR;
    
    // Initialize handle pool
    if (handle_pool_init(&g_pool) != 0) return IO_ERROR;
    
    atomic_store(&g_io.running, 1);
    atomic_store(&g_io.active_workers, 0);
    
    // Start worker threads
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pthread_create(&g_io.workers[i], NULL, worker_thread, NULL) == 0) {
            atomic_fetch_add(&g_io.active_workers, 1);
        }
    }
    
    return IO_OK;
}

int io_push_request(const char* json_request) {
    if (!atomic_load(&g_io.running) || !json_request) return IO_ERROR;
    
    // Parse JSON request
    uint32_t request_id = 0;
    uint32_t handle_id = 0;
    char method[32] = {0};
    char params[4096] = {0};
    
    // Simple JSON parsing - extract id
    const char* id_start = strstr(json_request, "\"id\":");
    if (id_start) {
        sscanf(id_start + 5, "%u", &request_id);
    }
    
    // Extract method
    const char* method_start = strstr(json_request, "\"method\":\"");
    if (method_start) {
        sscanf(method_start + 10, "%31[^\"]", method);
    }
    
    // Extract params
    const char* params_start = strstr(json_request, "\"params\":");
    if (params_start) {
        const char* params_start_brace = strchr(params_start + 9, '{');
        if (params_start_brace) {
            const char* params_end = strchr(params_start_brace, '}');
            if (params_end) {
                size_t len = params_end - params_start_brace + 1;
                if (len < sizeof(params)) {
                    strncpy(params, params_start_brace, len);
                    params[len] = '\0';
                }
            }
        }
    }
    
    // Handle_id is 0 for init, otherwise from params
    if (strcmp(method, "init") != 0) {
        const char* handle_start = strstr(params, "\"handle_id\":");
        if (handle_start) {
            sscanf(handle_start + 12, "%u", &handle_id);
        }
    }
    
    // Create queue item
    queue_item_t item = {
        .handle_id = handle_id,
        .request_id = request_id,
        .params = strdup(params),
        .params_len = strlen(params),
        .timestamp = (uint64_t)time(NULL)
    };
    strncpy(item.method, method, sizeof(item.method) - 1);
    
    if (queue_push(&g_io.request_queue, &item) != 0) {
        free(item.params);
        return IO_QUEUE_FULL;
    }
    
    return IO_OK;
}

int io_pop_response(char* json_response, size_t max_len) {
    if (!atomic_load(&g_io.running) || !json_response) return IO_ERROR;
    
    queue_item_t item;
    if (queue_pop(&g_io.response_queue, &item) != 0) {
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
    if (!atomic_load(&g_io.running)) return;
    
    atomic_store(&g_io.running, 0);
    
    // Wait for workers to finish with timeout
    int timeout = 3000; // 3 second timeout
    while (atomic_load(&g_io.active_workers) > 0 && timeout-- > 0) {
        usleep(1000);
    }
    
    if (atomic_load(&g_io.active_workers) > 0) {
        printf("⚠️  Warning: %d workers still active after shutdown timeout\n", 
               atomic_load(&g_io.active_workers));
    }
    
    // Clean up queues
    queue_item_t item;
    while (queue_pop(&g_io.request_queue, &item) == 0) {
        queue_item_cleanup(&item);
    }
    while (queue_pop(&g_io.response_queue, &item) == 0) {
        queue_item_cleanup(&item);
    }
}
