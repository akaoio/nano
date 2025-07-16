#pragma once

#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>

#define QUEUE_SIZE 1024
#define MAX_WORKERS 5
#define REQUEST_TIMEOUT_MS 30000

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
    _Atomic size_t head;
    _Atomic size_t tail;
    _Atomic size_t count;
} queue_t;

typedef struct {
    queue_t request_queue;
    queue_t response_queue;
    _Atomic int running;
} io_context_t;

// Queue s
int queue_init(queue_t* q);
int queue_push(queue_t* q, const queue_item_t* item);
int queue_pop(queue_t* q, queue_item_t* item);
int queue_empty(const queue_t* q);
int queue_full(const queue_t* q);
int queue_size(const queue_t* q);

// Thread-safe memory management
void queue_item_cleanup(queue_item_t* item);
