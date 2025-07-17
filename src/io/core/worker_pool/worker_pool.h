#pragma once

#include <stdint.h>
#include <pthread.h>
#include "../queue/queue.h"

typedef struct {
    pthread_t threads[MAX_WORKERS];
    volatile int running;
    queue_t* request_queue;
    queue_t* response_queue;
} worker_pool_t;

/**
 * @brief Initialize worker pool
 * @param pool Worker pool instance
 * @param req_q Request queue
 * @param resp_q Response queue
 * @return 0 on success, -1 on error
 */
int worker_pool_init(worker_pool_t* pool, queue_t* req_q, queue_t* resp_q);

/**
 * @brief Shutdown worker pool
 * @param pool Worker pool instance
 */
void worker_pool_shutdown(worker_pool_t* pool);
