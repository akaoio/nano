#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

int queue_init(queue_t* q) {
    if (!q) return -1;
    
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    
    for (size_t i = 0; i < QUEUE_SIZE; i++) {
        q->items[i].params = NULL;
        q->items[i].params_len = 0;
    }
    
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        return -1;
    }
    
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    
    if (pthread_cond_init(&q->not_full, NULL) != 0) {
        pthread_mutex_destroy(&q->mutex);
        pthread_cond_destroy(&q->not_empty);
        return -1;
    }
    
    return 0;
}

int queue_push(queue_t* q, const queue_item_t* item) {
    if (!q || !item) return -1;
    
    pthread_mutex_lock(&q->mutex);
    
    while (q->count >= QUEUE_SIZE) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
    
    queue_item_t* dest = &q->items[q->tail];
    
    if (dest->params) {
        free(dest->params);
        dest->params = NULL;
    }
    
    dest->handle_id = item->handle_id;
    dest->request_id = item->request_id;
    strncpy(dest->method, item->method, sizeof(dest->method) - 1);
    dest->method[sizeof(dest->method) - 1] = '\0';
    dest->timestamp = item->timestamp;
    
    if (item->params && item->params_len > 0) {
        dest->params = malloc(item->params_len + 1);
        if (!dest->params) {
            pthread_mutex_unlock(&q->mutex);
            return -1;
        }
        memcpy(dest->params, item->params, item->params_len);
        dest->params[item->params_len] = '\0';
        dest->params_len = item->params_len;
    } else {
        dest->params = NULL;
        dest->params_len = 0;
    }
    
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    q->count++;
    
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    
    return 0;
}

int queue_pop(queue_t* q, queue_item_t* item) {
    if (!q || !item) return -1;
    
    pthread_mutex_lock(&q->mutex);
    
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }
    
    const queue_item_t* src = &q->items[q->head];
    
    item->handle_id = src->handle_id;
    item->request_id = src->request_id;
    strncpy(item->method, src->method, sizeof(item->method) - 1);
    item->method[sizeof(item->method) - 1] = '\0';
    item->params = src->params;
    item->params_len = src->params_len;
    item->timestamp = src->timestamp;
    
    q->items[q->head].params = NULL;
    q->items[q->head].params_len = 0;
    
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->count--;
    
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    
    return 0;
}

int queue_pop_timeout(queue_t* q, queue_item_t* item, int timeout_ms) {
    if (!q || !item) return -1;
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    
    pthread_mutex_lock(&q->mutex);
    
    while (q->count == 0) {
        int ret = pthread_cond_timedwait(&q->not_empty, &q->mutex, &ts);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&q->mutex);
            return -2;
        }
    }
    
    const queue_item_t* src = &q->items[q->head];
    
    item->handle_id = src->handle_id;
    item->request_id = src->request_id;
    strncpy(item->method, src->method, sizeof(item->method) - 1);
    item->method[sizeof(item->method) - 1] = '\0';
    item->params = src->params;
    item->params_len = src->params_len;
    item->timestamp = src->timestamp;
    
    q->items[q->head].params = NULL;
    q->items[q->head].params_len = 0;
    
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->count--;
    
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    
    return 0;
}

int queue_empty(const queue_t* q) {
    if (!q) return 1;
    
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    int empty = (q->count == 0);
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    
    return empty;
}

int queue_full(const queue_t* q) {
    if (!q) return 1;
    
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    int full = (q->count >= QUEUE_SIZE);
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    
    return full;
}

int queue_size(const queue_t* q) {
    if (!q) return 0;
    
    pthread_mutex_lock((pthread_mutex_t*)&q->mutex);
    int size = (int)q->count;
    pthread_mutex_unlock((pthread_mutex_t*)&q->mutex);
    
    return size;
}

void queue_destroy(queue_t* q) {
    if (!q) return;
    
    pthread_mutex_lock(&q->mutex);
    
    for (size_t i = 0; i < QUEUE_SIZE; i++) {
        if (q->items[i].params) {
            free(q->items[i].params);
            q->items[i].params = NULL;
        }
    }
    
    pthread_mutex_unlock(&q->mutex);
    
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

void queue_item_cleanup(queue_item_t* item) {
    if (!item) return;
    
    if (item->params) {
        free(item->params);
        item->params = NULL;
    }
    item->params_len = 0;
}