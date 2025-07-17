#pragma once

#include "../queue/queue.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#define IO_OK 0
#define IO_ERROR -1
#define IO_TIMEOUT -2
#define IO_QUEUE_FULL -3

// Context structure for IO operations
typedef struct {
    queue_t request_queue;
    queue_t response_queue;
    pthread_t workers[MAX_WORKERS];
    _Atomic int running;
    _Atomic int active_workers;
} io_context_t;

/**
 * @brief Initialize IO system
 * @return IO_OK on success, IO_ERROR on failure
 */
int io_init(void);

/**
 * @brief Push JSON request to processing queue
 * @param json_request JSON-RPC request string
 * @return IO_OK on success, error code on failure
 */
int io_push_request(const char* json_request);

/**
 * @brief Pop JSON response from response queue
 * @param json_response Buffer to store response
 * @param max_len Maximum buffer length
 * @return IO_OK on success, error code on failure
 */
int io_pop_response(char* json_response, size_t max_len);

/**
 * @brief Shutdown IO system
 */
void io_shutdown(void);

// Internal functions
int io_parse_json_request(const char* json_request, uint32_t* request_id, 
                         uint32_t* handle_id, char* method, char* params);
void* io_worker_thread(void* arg);
