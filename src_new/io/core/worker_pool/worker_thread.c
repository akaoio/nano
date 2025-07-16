#include "worker_pool.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

// TODO: This should be replaced with actual operation execution
// For now, using a dummy implementation
static int execute_dummy_method(const char* method, uint32_t handle_id, 
                               const char* params, char* result, size_t result_size) {
    snprintf(result, result_size, "{\"status\":\"ok\",\"method\":\"%s\",\"handle\":%u}", 
             method, handle_id);
    return 0;
}

void* worker_thread(void* arg) {
    worker_pool_t* pool = (worker_pool_t*)arg;
    
    while (pool->running) {
        queue_item_t item;
        
        if (queue_pop(pool->request_queue, &item) == 0) {
            char result[8192];
            int ret = execute_dummy_method(item.method, item.handle_id, 
                                         item.params, result, sizeof(result));
            
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
            
            queue_item_cleanup(&item);
        } else {
            usleep(1000); // 1ms
        }
    }
    
    return NULL;
}
