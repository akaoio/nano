#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <stdatomic.h>
#include <pthread.h>

// Forward declarations - avoid circular dependencies
// MCP types are defined in their respective headers

// Types are defined in public headers - avoid duplication

/**
 * @file types.h
 * @brief Common type definitions used throughout the MCP server
 */

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations - actual definitions are in specific headers
// (These types are properly defined in mcp/protocol.h and mcp/server.h)

// Queue management types
#define QUEUE_SIZE 1024

typedef struct {
    uint32_t handle_id;
    uint32_t request_id;
    char method[32];
    char* params;
    size_t params_len;
    uint64_t timestamp;
} queue_item_t;

typedef struct {
    queue_item_t items[QUEUE_SIZE];
    size_t head;
    size_t tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} queue_t;

// IO and Worker types
#define REQUEST_TIMEOUT_MS 30000
#define IO_TIMEOUT -6
#define IO_OK 0
#define IO_ERROR -1
#define IO_QUEUE_FULL -2
#define MAX_WORKERS 4

// NANO callback type
typedef void (*nano_callback_t)(const char* response, void* userdata);

// Forward declarations for missing functions
void* io_worker_thread(void* arg);

// Operations functions
int io_operations_init(void);
void io_operations_shutdown(void);

// Forward declaration for streaming types
typedef struct stream_session stream_session_t;

// Streaming functions
int stream_add_chunk(const char* request_id, const char* delta, bool end, const char* error_msg);
stream_session_t* stream_create_session(const char* method, const char* request_id);

// IO context structure
typedef struct io_context {
    queue_t request_queue;
    atomic_bool running;
    atomic_int active_workers;
    pthread_t workers[MAX_WORKERS];
    nano_callback_t nano_callback;
    void* nano_userdata;
} io_context_t;

// Stream management types
#define REQUEST_ID_LENGTH 32
#define MAX_ACTIVE_STREAMS 64
#define STREAM_BUFFER_SIZE 8192
#define STREAM_SESSION_TIMEOUT_SEC 300

// Stream types are defined in the protocol files

// MCP context is defined in protocol headers

// Server config type is in mcp/server.h

// MCP Server structure is handled by server.h

#ifdef __cplusplus
}
#endif

#endif // COMMON_TYPES_H