#pragma once

// Core C23 utilities, constants, and common includes
// Unified header replacing common.h, constants.h, and c23_utils.h

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>

// Common utility modules
#include "memory_utils/memory_utils.h"
#include "string_utils/string_utils.h"
#include "error_utils/error_utils.h"
#include "handle_utils/handle_utils.h"
#include "transport_utils/transport_utils.h"

// System constants using C23 constexpr
constexpr int MAX_WORKERS = 5;
constexpr int QUEUE_SIZE = 1024;
constexpr int REQUEST_TIMEOUT_MS = 30000;
constexpr int MAX_HANDLES = 8;
constexpr int MAX_REQUEST_SIZE = 4096;
constexpr int MAX_RESPONSE_SIZE = 8192;

// Error codes
constexpr int ERR_OK = 0;
constexpr int ERR_INVALID_PARAM = -1;
constexpr int ERR_QUEUE_FULL = -2;
constexpr int ERR_QUEUE_EMPTY = -3;
constexpr int ERR_TIMEOUT = -4;
constexpr int ERR_MEMORY_LIMIT = -5;

// Note: Memory allocation functions are provided by memory_utils.h
// Use mem_alloc(), mem_realloc(), mem_free() for memory management


// Note: String comparison functions are provided by string_utils.h
// Use str_compare() for string operations

#define SAFE_INDEX(arr, idx, len) _Generic((arr), \
    char*: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr, \
    int*: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr, \
    uint32_t*: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr, \
    uint64_t*: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr, \
    default: ((idx) >= 0 && (idx) < (len)) ? &(arr)[idx] : nullptr)

// Note: String utilities are provided by string_utils.h
// Use str_copy(), str_length(), str_compare() for string operations

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

// C23 utility functions with attributes
// Note: is_valid_handle_id is defined in handle_utils.h to avoid duplication

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