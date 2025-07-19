#pragma once

#include "../queue/queue.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

// C23 compatibility
#if __STDC_VERSION__ >= 202311L
#define NODISCARD [[nodiscard]]
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define NODISCARD __attribute__((warn_unused_result))
#define MAYBE_UNUSED __attribute__((unused))
#endif

#define IO_OK 0
#define IO_ERROR -1
#define IO_TIMEOUT -2
#define IO_QUEUE_FULL -3

// Forward declaration for callback
typedef void (*nano_callback_t)(const char* json_response, void* userdata);

// Context structure for IO operations
typedef struct {
    queue_t request_queue;
    pthread_t workers[MAX_WORKERS];
    _Atomic int running;
    _Atomic int active_workers;
    nano_callback_t nano_callback;  // Direct callback to NANO
    void* nano_userdata;            // NANO context data
} io_context_t;

/**
 * @brief Initialize IO system with callback to NANO
 * @param callback Function to call when response is ready
 * @param userdata Data to pass to callback
 * @return IO_OK on success, IO_ERROR on failure
 */
NODISCARD int io_init(nano_callback_t callback, void* userdata);

/**
 * @brief Push JSON request to processing queue
 * @param json_request JSON-RPC request string
 * @return IO_OK on success, error code on failure
 */
NODISCARD int io_push_request(const char* json_request);

/**
 * @brief Set streaming callback for real-time responses
 * @param callback Function to call for streaming chunks
 * @param userdata Data to pass to callback
 */
void io_set_streaming_callback(nano_callback_t callback, void* userdata);

/**
 * @brief Shutdown IO system
 */
void io_shutdown(void);

// Internal functions
NODISCARD int io_parse_json_request(const char* json_request, uint32_t* request_id, 
                         uint32_t* handle_id, char* method, char* params);
void* io_worker_thread(void* arg);

// Streaming callback function
void io_streaming_chunk_callback(const char* chunk, bool is_final, void* userdata);
