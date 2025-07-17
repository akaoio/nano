#include "memory_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void* mem_alloc(size_t size) {
    if (size == 0) return NULL;
    
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed: %zu bytes\n", size);
        return NULL;
    }
    return ptr;
}

void* mem_realloc(void* ptr, size_t new_size) {
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        fprintf(stderr, "Memory reallocation failed: %zu bytes\n", new_size);
        return NULL;
    }
    return new_ptr;
}

void mem_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

int mem_pool_init(mem_pool_t* pool, size_t size, size_t block_size) {
    if (!pool || size == 0 || block_size == 0) return -1;
    
    pool->pool = malloc(size);
    if (!pool->pool) return -1;
    
    pool->size = size;
    pool->used = 0;
    pool->block_size = block_size;
    return 0;
}

void* mem_pool_alloc(mem_pool_t* pool) {
    if (!pool || !pool->pool) return NULL;
    
    if (pool->used + pool->block_size > pool->size) {
        return NULL; // Pool exhausted
    }
    
    void* ptr = (char*)pool->pool + pool->used;
    pool->used += pool->block_size;
    return ptr;
}

void mem_pool_free(mem_pool_t* pool, void* ptr) {
    // Simple pool implementation - no individual free
    // Would need more complex tracking for real free
    (void)pool;
    (void)ptr;
}

void mem_pool_destroy(mem_pool_t* pool) {
    if (pool && pool->pool) {
        free(pool->pool);
        pool->pool = NULL;
        pool->size = 0;
        pool->used = 0;
    }
}

void mem_zero(void* ptr, size_t size) {
    if (ptr && size > 0) {
        memset(ptr, 0, size);
    }
}

void mem_copy(void* dest, const void* src, size_t size) {
    if (dest && src && size > 0) {
        memcpy(dest, src, size);
    }
}

int mem_compare(const void* a, const void* b, size_t size) {
    if (!a || !b) return (a == b) ? 0 : -1;
    return memcmp(a, b, size);
}
