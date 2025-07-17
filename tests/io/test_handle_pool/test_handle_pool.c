#include "handle_pool.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void test_handle_pool_init() {
    printf("Testing handle_pool_init...\n");
    
    handle_pool_t pool;
    int result = handle_pool_init(&pool);
    
    assert(result == 0);
    assert(pool.next_id == 1);
    assert(pool.total_memory == 0);
    
    // Check all slots are inactive
    for (int i = 0; i < MAX_HANDLES; i++) {
        assert(pool.slots[i].active == false);
        assert(pool.slots[i].id == 0);
        assert(pool.slots[i].memory_usage == 0);
    }
    
    printf("handle_pool_init tests passed!\n");
}

void test_handle_pool_create_destroy() {
    printf("Testing handle_pool_create_destroy...\n");
    
    handle_pool_t pool;
    handle_pool_init(&pool);
    
    // Test create
    uint32_t handle_id = handle_pool_create(&pool, "/test/model.bin");
    assert(handle_id == 1);
    assert(handle_pool_is_valid(&pool, handle_id));
    
    // Test create another
    uint32_t handle_id2 = handle_pool_create(&pool, "/test/model2.bin");
    assert(handle_id2 == 2);
    assert(handle_pool_is_valid(&pool, handle_id2));
    
    // Test destroy
    int result = handle_pool_destroy(&pool, handle_id);
    assert(result == 0);
    assert(!handle_pool_is_valid(&pool, handle_id));
    assert(handle_pool_is_valid(&pool, handle_id2));
    
    // Test destroy second
    result = handle_pool_destroy(&pool, handle_id2);
    assert(result == 0);
    assert(!handle_pool_is_valid(&pool, handle_id2));
    
    printf("handle_pool_create_destroy tests passed!\n");
}

void test_handle_pool_get() {
    printf("Testing handle_pool_get...\n");
    
    handle_pool_t pool;
    handle_pool_init(&pool);
    
    uint32_t handle_id = handle_pool_create(&pool, "/test/model.bin");
    
    // Test get valid handle
    LLMHandle* handle = handle_pool_get(&pool, handle_id);
    assert(handle != NULL);
    
    // Test get invalid handle
    handle = handle_pool_get(&pool, 999);
    assert(handle == NULL);
    
    printf("handle_pool_get tests passed!\n");
}

void test_handle_pool_memory() {
    printf("Testing handle_pool_memory...\n");
    
    handle_pool_t pool;
    handle_pool_init(&pool);
    
    // Test initial memory
    assert(handle_pool_get_total_memory(&pool) == 0);
    
    uint32_t handle_id = handle_pool_create(&pool, "/test/model.bin");
    
    // Test memory functions (values start at 0)
    assert(handle_pool_get_memory_usage(&pool, handle_id) == 0);
    assert(handle_pool_get_total_memory(&pool) == 0);
    
    printf("handle_pool_memory tests passed!\n");
}

int main() {
    printf("Running handle_pool tests...\n");
    
    test_handle_pool_init();
    test_handle_pool_create_destroy();
    test_handle_pool_get();
    test_handle_pool_memory();
    
    printf("All handle_pool tests passed!\n");
    return 0;
}
