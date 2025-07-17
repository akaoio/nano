#include "handle_pool.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

// Global handle pool instance
handle_pool_t g_handle_pool;

int handle_pool_init(handle_pool_t* pool) {
    if (!pool) return -1;
    
    memset(pool, 0, sizeof(*pool));
    pool->next_id = 1;
    pool->total_memory = 0;
    
    for (int i = 0; i < MAX_HANDLES; i++) {
        pool->slots[i].id = 0;
        pool->slots[i].active = false;
        pool->slots[i].handle = NULL;
        pool->slots[i].memory_usage = 0;
        pool->slots[i].last_used = 0;
    }
    
    return 0;
}

uint32_t handle_pool_create(handle_pool_t* pool, const char* model_path) {
    if (!pool || !model_path) return 0;
    
    // Find first available slot
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (!pool->slots[i].active) {
            pool->slots[i].id = pool->next_id++;
            pool->slots[i].active = true;
            strncpy(pool->slots[i].model_path, model_path, sizeof(pool->slots[i].model_path) - 1);
            pool->slots[i].model_path[sizeof(pool->slots[i].model_path) - 1] = '\0';
            pool->slots[i].memory_usage = 0;
            pool->slots[i].last_used = (uint64_t)time(NULL);
            pool->slots[i].handle = NULL; // Will be set by rkllm_init
            
            return pool->slots[i].id;
        }
    }
    
    return 0; // No available slots
}

int handle_pool_destroy(handle_pool_t* pool, uint32_t handle_id) {
    if (!pool || handle_id == 0) return -1;
    
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (pool->slots[i].active && pool->slots[i].id == handle_id) {
            pool->slots[i].active = false;
            pool->slots[i].id = 0;
            pool->slots[i].handle = NULL;
            pool->total_memory -= pool->slots[i].memory_usage;
            pool->slots[i].memory_usage = 0;
            return 0;
        }
    }
    
    return -1; // Handle not found
}

int handle_pool_set_handle(handle_pool_t* pool, uint32_t handle_id, LLMHandle handle) {
    if (!pool || handle_id == 0) return -1;
    
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (pool->slots[i].active && pool->slots[i].id == handle_id) {
            pool->slots[i].handle = handle;
            pool->slots[i].last_used = (uint64_t)time(NULL);
            return 0;
        }
    }
    
    return -1; // Handle not found
}

LLMHandle* handle_pool_get(handle_pool_t* pool, uint32_t handle_id) {
    if (!pool || handle_id == 0) return NULL;
    
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (pool->slots[i].active && pool->slots[i].id == handle_id) {
            pool->slots[i].last_used = (uint64_t)time(NULL);
            return &pool->slots[i].handle;
        }
    }
    
    return NULL;
}

bool handle_pool_is_valid(handle_pool_t* pool, uint32_t handle_id) {
    if (!pool || handle_id == 0) return false;
    
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (pool->slots[i].active && pool->slots[i].id == handle_id) {
            return true;
        }
    }
    
    return false;
}

size_t handle_pool_get_memory_usage(handle_pool_t* pool, uint32_t handle_id) {
    if (!pool || handle_id == 0) return 0;
    
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (pool->slots[i].active && pool->slots[i].id == handle_id) {
            return pool->slots[i].memory_usage;
        }
    }
    
    return 0;
}

size_t handle_pool_get_total_memory(handle_pool_t* pool) {
    if (!pool) return 0;
    return pool->total_memory;
}
