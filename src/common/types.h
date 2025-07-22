#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Use transport_type_t from mcp/transport.h to avoid conflicts

// Connection states
typedef enum {
    CONNECTION_STATE_DISCONNECTED = 0,
    CONNECTION_STATE_CONNECTING,
    CONNECTION_STATE_CONNECTED,
    CONNECTION_STATE_ERROR
} connection_state_t;

// Generic result codes
typedef enum {
    RESULT_SUCCESS = 0,
    RESULT_ERROR = -1,
    RESULT_TIMEOUT = -2,
    RESULT_INVALID_PARAM = -3,
    RESULT_OUT_OF_MEMORY = -4,
    RESULT_NOT_INITIALIZED = -5
} result_code_t;

#endif // COMMON_TYPES_H