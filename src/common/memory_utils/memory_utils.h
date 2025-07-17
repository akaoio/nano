#pragma once

#include <stddef.h>
#include <stdint.h>

// Memory allocation with error checking
void* mem_alloc(size_t size);
void* mem_realloc(void* ptr, size_t new_size);
void mem_free(void* ptr);

// Memory pool for frequent allocations
typedef struct {
    void* pool;
    size_t size;
    size_t used;
    size_t block_size;
} mem_pool_t;

int mem_pool_init(mem_pool_t* pool, size_t size, size_t block_size);
void* mem_pool_alloc(mem_pool_t* pool);
void mem_pool_free(mem_pool_t* pool, void* ptr);
void mem_pool_destroy(mem_pool_t* pool);

// Memory utilities
void mem_zero(void* ptr, size_t size);
void mem_copy(void* dest, const void* src, size_t size);
int mem_compare(const void* a, const void* b, size_t size);
