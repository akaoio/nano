#pragma once

#include <stdint.h>
#include <pthread.h>
#include "queue.h"
#include "handle_pool.h"

#define MAX_WORKERS 5

typedef struct {
    pthread_t threads[MAX_WORKERS];
    volatile int running;
    queue_t* request_queue;
    queue_t* response_queue;
} worker_pool_t;

int worker_pool_init(worker_pool_t* pool, queue_t* req_q, queue_t* resp_q);
void worker_pool_shutdown(worker_pool_t* pool);
