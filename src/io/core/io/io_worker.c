#define _GNU_SOURCE
#include "io.h"
#include "../../operations.h"
#include "../../../common/core.h"
#include <json-c/json.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
            // Create timeout response using json-c
            json_object *response = json_object_new_object();
            json_object *jsonrpc = json_object_new_string("2.0");
            json_object *id = json_object_new_int(item.request_id);
            json_object *error = json_object_new_object();
            json_object *code = json_object_new_int(IO_TIMEOUT);
            json_object *message = json_object_new_string("Request timeout");
            
            json_object_object_add(error, "code", code);
            json_object_object_add(error, "message", message);
            json_object_object_add(response, "jsonrpc", jsonrpc);
            json_object_object_add(response, "id", id);
            json_object_object_add(response, "error", error);
            
            const char *response_str = json_object_to_json_string(response);
            
            queue_item_t resp_item = {
                .handle_id = item.handle_id,
                .request_id = item.request_id,
                .params = strdup(response_str),
                .params_len = strlen(response_str),
                .timestamp = now
            };
            strncpy(resp_item.method, "response", sizeof(resp_item.method) - 1);
            
            queue_push(&g_io_context.response_queue, &resp_item);
            json_object_put(response);
            queue_item_cleanup(&item);
            continue;
        }
        
        // Process the request
        // Reconstruct full JSON-RPC request using json-c
        json_object *request = json_object_new_object();
        json_object *jsonrpc = json_object_new_string("2.0");
        json_object *id = json_object_new_int(item.request_id);
        json_object *method = json_object_new_string(item.method);
        json_object *params = json_tokener_parse(item.params);
        if (!params) {
            params = json_object_new_string(item.params);
        }
        
        json_object_object_add(request, "jsonrpc", jsonrpc);
        json_object_object_add(request, "id", id);
        json_object_object_add(request, "method", method);
        json_object_object_add(request, "params", params);
        
        const char *json_request_str = json_object_to_json_string(request);
        
        // Process request
        char response[16384];
        (void)io_process_request(json_request_str, response, sizeof(response));
        
        json_object_put(request);
        
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
