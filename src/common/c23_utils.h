#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// Pure C23 implementation - no compatibility layer needed

// Type-safe utility macros using C23 _Generic
#define SAFE_MALLOC(type, count) _Generic((type*)0, \
    char*: (type*)malloc(sizeof(type) * (count)), \
    int*: (type*)malloc(sizeof(type) * (count)), \
    uint32_t*: (type*)malloc(sizeof(type) * (count)), \
    uint64_t*: (type*)malloc(sizeof(type) * (count)), \
    default: (type*)malloc(sizeof(type) * (count)))

#define SAFE_REALLOC(ptr, type, count) _Generic((ptr), \
    char*: (type*)realloc(ptr, sizeof(type) * (count)), \
    int*: (type*)realloc(ptr, sizeof(type) * (count)), \
    uint32_t*: (type*)realloc(ptr, sizeof(type) * (count)), \
    uint64_t*: (type*)realloc(ptr, sizeof(type) * (count)), \
    default: (type*)realloc(ptr, sizeof(type) * (count)))

#define SAFE_MEMCPY(dest, src, count) _Generic((dest), \
    char*: memcpy(dest, src, (count) * sizeof(*(dest))), \
    int*: memcpy(dest, src, (count) * sizeof(*(dest))), \
    uint32_t*: memcpy(dest, src, (count) * sizeof(*(dest))), \
    uint64_t*: memcpy(dest, src, (count) * sizeof(*(dest))), \
    default: memcpy(dest, src, (count) * sizeof(*(dest))))

#define SAFE_ZERO(ptr, count) _Generic((ptr), \
    char*: memset(ptr, 0, (count) * sizeof(*(ptr))), \
    int*: memset(ptr, 0, (count) * sizeof(*(ptr))), \
    uint32_t*: memset(ptr, 0, (count) * sizeof(*(ptr))), \
    uint64_t*: memset(ptr, 0, (count) * sizeof(*(ptr))), \
    default: memset(ptr, 0, (count) * sizeof(*(ptr))))

#define SAFE_COMPARE(a, b) _Generic((a), \
    char*: strcmp(a, b), \
    const char*: strcmp(a, b), \
    int: ((a) == (b)) ? 0 : ((a) < (b)) ? -1 : 1, \
    uint32_t: ((a) == (b)) ? 0 : ((a) < (b)) ? -1 : 1, \
    uint64_t: ((a) == (b)) ? 0 : ((a) < (b)) ? -1 : 1, \
    default: memcmp(&(a), &(b), sizeof(a)))

#define SAFE_INDEX(arr, idx, len) _Generic((arr), \
    char*: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr, \
    int*: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr, \
    uint32_t*: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr, \
    uint64_t*: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr, \
    default: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr)

#define SAFE_STRDUP(str) _Generic((str), \
    char*: strdup(str), \
    const char*: strdup(str), \
    default: strdup(str))

#define SAFE_STRLEN(str) _Generic((str), \
    char*: strlen(str), \
    const char*: strlen(str), \
    default: strlen(str))

// Additional utility macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define IS_VALID_PTR(ptr) ((ptr) != nullptr)

// Error handling with type safety
#define RETURN_IF_NULL(ptr) do { \
    if (!IS_VALID_PTR(ptr)) { \
        return ERR_INVALID_PARAM; \
    } \
} while(0)

#define RETURN_NULL_IF_NULL(ptr) do { \
    if (!IS_VALID_PTR(ptr)) { \
        return nullptr; \
    } \
} while(0)

// Pure C23 utility functions with attributes
[[nodiscard]] static inline bool is_valid_handle_id(uint32_t id) {
    return id > 0 && id < UINT32_MAX;
}

[[nodiscard]] static inline bool is_valid_buffer_size(size_t size) {
    return size > 0 && size <= MAX_REQUEST_SIZE;
}

[[nodiscard]] static inline bool is_in_range(int value, int min, int max) {
    return value >= min && value <= max;
}

// C23 auto type deduction helpers
#define DECLARE_AUTO(name, value) auto name = (value)
#define FOR_AUTO(var, init, condition, increment) \
    for (auto var = (init); condition; increment)