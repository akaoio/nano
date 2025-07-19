#include "string_utils.h"
#include "../memory_utils/memory_utils.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

char* str_copy(const char* src) {
    if (!src) return nullptr;
    
    size_t len = strlen(src);
    char* dest = mem_alloc(len + 1);
    if (!dest) return nullptr;
    
    memcpy(dest, src, len + 1);
    return dest;
}


void str_free(char* str) {
    mem_free(str);
}

int str_length(const char* str) {
    return str ? strlen(str) : 0;
}

int str_compare(const char* a, const char* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strcmp(a, b);
}

int str_compare_n(const char* a, const char* b, size_t n) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strncmp(a, b, n);
}

bool str_equals(const char* a, const char* b) {
    return str_compare(a, b) == 0;
}

bool str_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool str_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) return false;
    
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}



int str_builder_init(str_builder_t* builder, size_t initial_capacity) {
    if (!builder) return -1;
    
    builder->buffer = mem_alloc(initial_capacity);
    if (!builder->buffer) return -1;
    
    builder->buffer[0] = '\0';
    builder->size = 0;
    builder->capacity = initial_capacity;
    return 0;
}

// Helper function for capacity expansion
static int str_builder_ensure_capacity(str_builder_t* builder, size_t needed_size) {
    if (builder->size + needed_size + 1 <= builder->capacity) {
        return 0; // Already enough capacity
    }
    
    size_t new_capacity = builder->capacity * 2;
    if (new_capacity < builder->size + needed_size + 1) {
        new_capacity = builder->size + needed_size + 1;
    }
    
    char* new_buffer = mem_realloc(builder->buffer, new_capacity);
    if (!new_buffer) return -1;
    
    builder->buffer = new_buffer;
    builder->capacity = new_capacity;
    return 0;
}

int str_builder_append(str_builder_t* builder, const char* str) {
    if (!builder || !str) return -1;
    
    size_t str_len = strlen(str);
    if (str_builder_ensure_capacity(builder, str_len) != 0) {
        return -1;
    }
    
    memcpy(builder->buffer + builder->size, str, str_len);
    builder->size += str_len;
    builder->buffer[builder->size] = '\0';
    
    return 0;
}

int str_builder_append_format(str_builder_t* builder, const char* format, ...) {
    if (!builder || !format) return -1;
    
    va_list args;
    va_start(args, format);
    
    // Calculate required size
    int needed = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    
    if (needed < 0) return -1;
    
    // Ensure capacity
    if (str_builder_ensure_capacity(builder, needed) != 0) {
        return -1;
    }
    
    // Format string
    va_start(args, format);
    vsnprintf(builder->buffer + builder->size, needed + 1, format, args);
    va_end(args);
    
    builder->size += needed;
    return 0;
}


void str_builder_destroy(str_builder_t* builder) {
    if (builder) {
        mem_free(builder->buffer);
        builder->buffer = nullptr;
        builder->size = 0;
        builder->capacity = 0;
    }
}



