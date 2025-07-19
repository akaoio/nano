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

// Streaming callback function for RKLLM (non-static for external use)
void io_streaming_chunk_callback(const char* chunk, bool is_final, void* userdata) {
    if (!chunk || !userdata) return;
    
    uint32_t* request_id = (uint32_t*)userdata;
    
    // Create streaming JSON response
    json_object *response = json_object_new_object();
    json_object *jsonrpc = json_object_new_string("2.0");
    json_object *id = json_object_new_int(*request_id);
    json_object *result = json_object_new_object();
    
    // Add streaming data
    json_object *data = json_object_new_string(chunk);
    json_object *final = json_object_new_boolean(is_final);
    
    json_object_object_add(result, "data", data);
    json_object_object_add(result, "final", final);
    json_object_object_add(response, "jsonrpc", jsonrpc);
    json_object_object_add(response, "id", id);
    json_object_object_add(response, "result", result);
    
    const char *response_str = json_object_to_json_string(response);
    
    // Call NANO directly for streaming chunks
    if (g_io_context.nano_callback) {
        g_io_context.nano_callback(response_str, g_io_context.nano_userdata);
    }
    
    json_object_put(response);
}

void* io_worker_thread(void* arg) {
    (void)arg;
    printf("\n🔧 === IO WORKER THREAD STARTED ===\n");
    
    while (atomic_load(&g_io_context.running)) {
        queue_item_t item;
        
        // Try to get a request
        if (queue_pop(&g_io_context.request_queue, &item) != 0) {
            sleep(1); // 1 second sleep when no work
            continue;
        }
        
        printf("\n🔧 === IO WORKER GOT REQUEST ===\n");
        printf("📥 Processing: ID=%d, Method=%s\n", item.request_id, item.method);
        
        // Check for timeout
        uint64_t now = (uint64_t)time(nullptr);
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
            
            // Call NANO directly instead of pushing to queue
            if (g_io_context.nano_callback) {
                g_io_context.nano_callback(response_str, g_io_context.nano_userdata);
            }
            
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
        
        // Call NANO directly instead of pushing to queue
        printf("\n🔧 === IO WORKER CALLBACK ===\n");
        printf("📤 Sending to NANO: %s\n", response);
        if (g_io_context.nano_callback) {
            printf("✅ Calling NANO callback\n");
            g_io_context.nano_callback(response, g_io_context.nano_userdata);
        } else {
            printf("❌ NANO callback is NULL!\n");
        }
        
        queue_item_cleanup(&item);
    }
    
    atomic_fetch_sub(&g_io_context.active_workers, 1);
    return nullptr;
}
