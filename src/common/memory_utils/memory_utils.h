#pragma once

#include <stddef.h>
#include <stdint.h>

// Memory allocation with error checking
void* mem_alloc(size_t size);
void* mem_realloc(void* ptr, size_t new_size);
void mem_free(void* ptr);


