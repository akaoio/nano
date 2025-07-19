#define _GNU_SOURCE
#include "worker_pool.h"
#include "../queue/queue.h"
#include "../../operations.h"
#include "../../../common/core.h"
#include <json-c/json.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Forward declaration
static void* worker_thread(void* arg);

int worker_pool_init(worker_pool_t* pool, queue_t* req_q, queue_t* resp_q) {
    if (!pool || !req_q || !resp_q) return -1;
    
    pool->running = 1;
    pool->request_queue = req_q;
    pool->response_queue = resp_q;
    
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pthread_create(&pool->threads[i], nullptr, worker_thread, pool) != 0) {
            // Cleanup on failure
            pool->running = 0;
            return -1;
        }
        pthread_detach(pool->threads[i]);
    }
    
    return 0;
}

void worker_pool_shutdown(worker_pool_t* pool) {
    if (!pool) return;
    pool->running = 0;
    
    // Give threads time to exit
    usleep(10000); // 10ms
}

static void* worker_thread(void* arg) {
    worker_pool_t* pool = (worker_pool_t*)arg;
    
    while (pool->running) {
        queue_item_t item;
        
        if (queue_pop(pool->request_queue, &item) == 0) {
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
            
            // Process request using io_process_request
            char response[16384];
            io_process_request(json_request_str, response, sizeof(response));
            
            json_object_put(request);
            
            // Push to response queue
            queue_item_t resp_item = {
                .handle_id = item.handle_id,
                .request_id = item.request_id,
                .params = strdup(response),
                .params_len = strlen(response)
            };
            strncpy(resp_item.method, "response", sizeof(resp_item.method) - 1);
            resp_item.method[sizeof(resp_item.method) - 1] = '\0';
            
            while (queue_push(pool->response_queue, &resp_item) != 0) {
                usleep(1000);
            }
            
            // Clean up request item
            queue_item_cleanup(&item);
        } else {
            usleep(1000); // 1ms
        }
    }
    
    return nullptr;
}
