#define _GNU_SOURCE
#include "worker_pool.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

void test_worker_pool_init() {
    worker_pool_t pool;
    queue_t req_queue, resp_queue;
    
    // Initialize queues
    assert(queue_init(&req_queue) == 0);
    assert(queue_init(&resp_queue) == 0);
    
    // Test worker pool initialization
    assert(worker_pool_init(&pool, &req_queue, &resp_queue) == 0);
    assert(pool.running == 1);
    assert(pool.request_queue == &req_queue);
    assert(pool.response_queue == &resp_queue);
    
    // Cleanup
    worker_pool_shutdown(&pool);
    
    printf("✓ test_worker_pool_init passed\n");
}

void test_worker_pool_processing() {
    worker_pool_t pool;
    queue_t req_queue, resp_queue;
    
    // Initialize queues
    assert(queue_init(&req_queue) == 0);
    assert(queue_init(&resp_queue) == 0);
    
    // Initialize worker pool
    assert(worker_pool_init(&pool, &req_queue, &resp_queue) == 0);
    
    // Create a test request
    queue_item_t request = {
        .handle_id = 1,
        .request_id = 123,
        .params = strdup("{\"test\":\"data\"}"),
        .params_len = 15,
        .timestamp = time(NULL)
    };
    strncpy(request.method, "test_method", sizeof(request.method) - 1);
    request.method[sizeof(request.method) - 1] = '\0';
    
    // Push request
    assert(queue_push(&req_queue, &request) == 0);
    
    // Wait for processing
    usleep(50000); // 50ms
    
    // Check if response was generated
    queue_item_t response;
    int got_response = 0;
    for (int i = 0; i < 10; i++) {
        if (queue_pop(&resp_queue, &response) == 0) {
            got_response = 1;
            break;
        }
        usleep(10000); // 10ms
    }
    
    assert(got_response == 1);
    assert(response.request_id == 123);
    assert(response.params != NULL);
    printf("Response: %s\n", response.params);
    
    // Cleanup
    queue_item_cleanup(&response);
    worker_pool_shutdown(&pool);
    
    printf("✓ test_worker_pool_processing passed\n");
}

void test_worker_pool_shutdown() {
    worker_pool_t pool;
    queue_t req_queue, resp_queue;
    
    // Initialize queues
    assert(queue_init(&req_queue) == 0);
    assert(queue_init(&resp_queue) == 0);
    
    // Initialize worker pool
    assert(worker_pool_init(&pool, &req_queue, &resp_queue) == 0);
    assert(pool.running == 1);
    
    // Shutdown
    worker_pool_shutdown(&pool);
    assert(pool.running == 0);
    
    printf("✓ test_worker_pool_shutdown passed\n");
}

int main() {
    printf("Running worker_pool tests...\n");
    
    test_worker_pool_init();
    test_worker_pool_processing();
    test_worker_pool_shutdown();
    
    printf("All worker_pool tests passed!\n");
    return 0;
}
