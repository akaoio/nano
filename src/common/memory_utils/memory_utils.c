#include "memory_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void* mem_alloc(size_t size) {
    if (size == 0) return nullptr;
    
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed: %zu bytes\n", size);
        return nullptr;
    }
    return ptr;
}

void* mem_realloc(void* ptr, size_t new_size) {
    if (new_size == 0) {
        free(ptr);
        return nullptr;
    }
    
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        fprintf(stderr, "Memory reallocation failed: %zu bytes\n", new_size);
        return nullptr;
    }
    return new_ptr;
}

void mem_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}







