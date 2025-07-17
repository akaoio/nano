#pragma once

#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>

#include "../../../common/core.h"

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

/**
 * @brief Initialize a queue
 * @param q Queue to initialize
 * @return 0 on success, -1 on error
 */
int queue_init(queue_t* q);

/**
 * @brief Push item to queue
 * @param q Queue to push to
 * @param item Item to push
 * @return 0 on success, -1 on error
 */
int queue_push(queue_t* q, const queue_item_t* item);

/**
 * @brief Pop item from queue
 * @param q Queue to pop from
 * @param item Item to store result
 * @return 0 on success, -1 on error
 */
int queue_pop(queue_t* q, queue_item_t* item);

/**
 * @brief Check if queue is empty
 * @param q Queue to check
 * @return 1 if empty, 0 otherwise
 */
int queue_empty(const queue_t* q);

/**
 * @brief Check if queue is full
 * @param q Queue to check
 * @return 1 if full, 0 otherwise
 */
int queue_full(const queue_t* q);

/**
 * @brief Get queue size
 * @param q Queue to check
 * @return Number of items in queue
 */
int queue_size(const queue_t* q);

/**
 * @brief Clean up queue item memory
 * @param item Item to clean up
 */
void queue_item_cleanup(queue_item_t* item);
