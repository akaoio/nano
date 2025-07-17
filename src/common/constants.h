#ifndef CONSTANTS_H
#define CONSTANTS_H

// Pure C23 implementation - no fallback support

// Worker pool configuration
constexpr int MAX_WORKERS = 5;

// Queue configuration
constexpr int QUEUE_SIZE = 1024;
constexpr int REQUEST_TIMEOUT_MS = 30000;

// Handle pool configuration
constexpr int MAX_HANDLES = 8;

// Memory limits
constexpr int MAX_REQUEST_SIZE = 4096;
constexpr int MAX_RESPONSE_SIZE = 8192;

// Error codes
constexpr int ERR_OK = 0;
constexpr int ERR_INVALID_PARAM = -1;
constexpr int ERR_QUEUE_FULL = -2;
constexpr int ERR_QUEUE_EMPTY = -3;
constexpr int ERR_TIMEOUT = -4;
constexpr int ERR_MEMORY_LIMIT = -5;

// Additional type-safe utilities are defined in c23_utils.h
#include "c23_utils.h"

#endif /* CONSTANTS_H */