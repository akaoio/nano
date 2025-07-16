#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

int queue_init(queue_t* q) {
    if (!q) return -1;
    
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    
    // Initialize all items
    for (size_t i = 0; i < QUEUE_SIZE; i++) {
        q->items[i].params = NULL;
        q->items[i].params_len = 0;
    }
    
    return 0;
}

int queue_push(queue_t* q, const queue_item_t* item) {
    if (!q || !item) return -1;
    
    size_t count = atomic_load(&q->count);
    if (count >= QUEUE_SIZE) {
        return -1; // Queue full
    }
    
    size_t tail = atomic_load(&q->tail);
    queue_item_t* dest = &q->items[tail];
    
    // Copy data
    dest->handle_id = item->handle_id;
    dest->request_id = item->request_id;
    strncpy(dest->method, item->method, sizeof(dest->method) - 1);
    dest->method[sizeof(dest->method) - 1] = '\0';
    dest->timestamp = item->timestamp;
    
    if (item->params && item->params_len > 0) {
        dest->params = malloc(item->params_len + 1);
        if (!dest->params) return -1;
        memcpy(dest->params, item->params, item->params_len);
        dest->params[item->params_len] = '\0';
        dest->params_len = item->params_len;
    } else {
        dest->params = NULL;
        dest->params_len = 0;
    }
    
    // Update tail and count atomically
    atomic_store(&q->tail, (tail + 1) % QUEUE_SIZE);
    atomic_fetch_add(&q->count, 1);
    
    return 0;
}

int queue_pop(queue_t* q, queue_item_t* item) {
    if (!q || !item) return -1;
    
    size_t count = atomic_load(&q->count);
    if (count == 0) {
        return -1; // Queue empty
    }
    
    size_t head = atomic_load(&q->head);
    const queue_item_t* src = &q->items[head];
    
    // Copy data
    item->handle_id = src->handle_id;
    item->request_id = src->request_id;
    strncpy(item->method, src->method, sizeof(item->method) - 1);
    item->method[sizeof(item->method) - 1] = '\0';
    item->params = src->params;
    item->params_len = src->params_len;
    item->timestamp = src->timestamp;
    
    // Update head and count atomically
    atomic_store(&q->head, (head + 1) % QUEUE_SIZE);
    atomic_fetch_sub(&q->count, 1);
    
    // Clear the slot
    q->items[head].params = NULL;
    q->items[head].params_len = 0;
    
    return 0;
}

int queue_empty(const queue_t* q) {
    if (!q) return 1;
    return atomic_load(&q->count) == 0;
}

int queue_full(const queue_t* q) {
    if (!q) return 1;
    return atomic_load(&q->count) >= QUEUE_SIZE;
}

int queue_size(const queue_t* q) {
    if (!q) return 0;
    return (int)atomic_load(&q->count);
}

void queue_item_cleanup(queue_item_t* item) {
    if (!item) return;
    
    if (item->params) {
        free(item->params);
        item->params = NULL;
    }
    item->params_len = 0;
}
