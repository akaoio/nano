#include "worker_pool.h"
#include "operations.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static void* worker_thread(void* arg) {
    worker_pool_t* pool = (worker_pool_t*)arg;
    
    while (pool->running) {
        queue_item_t item;
        
        if (queue_pop(pool->request_queue, &item) == 0) {
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
                .params_len = strlen(response)
            };
            strncpy(resp_item.method, "response", sizeof(resp_item.method) - 1);
            resp_item.method[sizeof(resp_item.method) - 1] = '\0';
            
            while (queue_push(pool->response_queue, &resp_item) != 0) {
                usleep(1000);
            }
            
            free(item.params);
        } else {
            usleep(1000); // 1ms
        }
    }
    
    return NULL;
}

int worker_pool_init(worker_pool_t* pool, queue_t* req_q, queue_t* resp_q) {
    pool->running = 1;
    pool->request_queue = req_q;
    pool->response_queue = resp_q;
    
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            return -1;
        }
        pthread_detach(pool->threads[i]);
    }
    
    return 0;
}

void worker_pool_shutdown(worker_pool_t* pool) {
    pool->running = 0;
}
