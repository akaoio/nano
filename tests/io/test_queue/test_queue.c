#include "queue.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void test_queue_init() {
    printf("Testing queue_init...\n");
    
    queue_t q;
    int result = queue_init(&q);
    assert(result == 0);
    assert(q.head == 0);
    assert(q.tail == 0);
    assert(q.count == 0);
    
    printf("queue_init tests passed!\n");
}

void test_queue_push_pop() {
    printf("Testing queue_push_pop...\n");
    
    queue_t q;
    queue_init(&q);
    
    queue_item_t item1 = {
        .handle_id = 1,
        .request_id = 100,
        .method = "test_method",
        .params = "test_params",
        .params_len = 11,
        .timestamp = 12345
    };
    
    // Test push
    int result = queue_push(&q, &item1);
    assert(result == 0);
    assert(queue_size(&q) == 1);
    assert(!queue_empty(&q));
    
    // Test pop
    queue_item_t item2;
    result = queue_pop(&q, &item2);
    assert(result == 0);
    assert(item2.handle_id == 1);
    assert(item2.request_id == 100);
    assert(strcmp(item2.method, "test_method") == 0);
    assert(item2.params_len == 11);
    assert(strcmp(item2.params, "test_params") == 0);
    assert(item2.timestamp == 12345);
    
    // Clean up
    queue_item_cleanup(&item2);
    
    printf("queue_push_pop tests passed!\n");
}

void test_queue_empty_full() {
    printf("Testing queue_empty_full...\n");
    
    queue_t q;
    queue_init(&q);
    
    // Test empty
    assert(queue_empty(&q));
    assert(!queue_full(&q));
    assert(queue_size(&q) == 0);
    
    // Fill queue partially
    queue_item_t item = {.handle_id = 1, .request_id = 100};
    queue_push(&q, &item);
    
    assert(!queue_empty(&q));
    assert(!queue_full(&q));
    assert(queue_size(&q) == 1);
    
    printf("queue_empty_full tests passed!\n");
}

int main() {
    printf("Running queue tests...\n");
    
    test_queue_init();
    test_queue_push_pop();
    test_queue_empty_full();
    
    printf("All queue tests passed!\n");
    return 0;
}
