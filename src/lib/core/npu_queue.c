#include "npu_queue.h"
#include "async_response.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// External worker thread function
extern void* npu_worker_thread(void* arg);

// Global NPU queue instance
npu_queue_t* g_npu_queue = NULL;

int npu_queue_init(npu_queue_t* queue, int max_queue_size) {
    if (!queue) {
        return -1;
    }
    
    memset(queue, 0, sizeof(npu_queue_t));
    
    // Allocate task queue
    queue->task_queue = malloc(max_queue_size * sizeof(npu_task_t));
    if (!queue->task_queue) {
        printf("❌ NPU Queue: Failed to allocate task queue\n");
        return -1;
    }
    
    queue->queue_capacity = max_queue_size;
    queue->queue_size = 0;
    queue->queue_head = 0;
    queue->queue_tail = 0;
    
    // Initialize synchronization primitives
    if (pthread_mutex_init(&queue->queue_mutex, NULL) != 0) {
        printf("❌ NPU Queue: Failed to initialize mutex\n");
        free(queue->task_queue);
        return -1;
    }
    
    if (pthread_cond_init(&queue->queue_cond, NULL) != 0) {
        printf("❌ NPU Queue: Failed to initialize condition variable\n");
        pthread_mutex_destroy(&queue->queue_mutex);
        free(queue->task_queue);
        return -1;
    }
    
    if (pthread_cond_init(&queue->shutdown_cond, NULL) != 0) {
        printf("❌ NPU Queue: Failed to initialize shutdown condition variable\n");
        pthread_cond_destroy(&queue->queue_cond);
        pthread_mutex_destroy(&queue->queue_mutex);
        free(queue->task_queue);
        return -1;
    }
    
    // Initialize state
    queue->running = true;
    queue->npu_busy = false;
    queue->shutdown_requested = false;
    queue->tasks_processed = 0;
    queue->tasks_failed = 0;
    queue->queue_overflows = 0;
    
    // Start worker thread
    if (pthread_create(&queue->worker_thread, NULL, npu_worker_thread, queue) != 0) {
        printf("❌ NPU Queue: Failed to create worker thread\n");
        pthread_cond_destroy(&queue->shutdown_cond);
        pthread_cond_destroy(&queue->queue_cond);
        pthread_mutex_destroy(&queue->queue_mutex);
        free(queue->task_queue);
        return -1;
    }
    
    printf("✅ NPU Queue: Initialized with capacity %d\n", max_queue_size);
    return 0;
}

int npu_queue_add_task(npu_queue_t* queue, const npu_task_t* task) {
    if (!queue || !task) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->queue_mutex);
    
    // Check if queue is full
    if (queue->queue_size >= queue->queue_capacity) {
        queue->queue_overflows++;
        pthread_mutex_unlock(&queue->queue_mutex);
        printf("⚠️  NPU Queue: Queue full, rejecting task %s (ID: %s)\n", 
               task->method, task->request_id);
        return -2; // Queue full error
    }
    
    // Add task to queue
    npu_task_t* queued_task = &queue->task_queue[queue->queue_tail];
    
    // Deep copy task data
    queued_task->method = strdup(task->method);
    queued_task->params_json = strdup(task->params_json);
    strncpy(queued_task->request_id, task->request_id, NPU_TASK_REQUEST_ID_SIZE - 1);
    queued_task->request_id[NPU_TASK_REQUEST_ID_SIZE - 1] = '\0';
    queued_task->transport_index = task->transport_index;
    queued_task->connection_handle = task->connection_handle;
    queued_task->queued_at = time(NULL);
    queued_task->op_type = task->op_type;
    
    // Update queue state
    queue->queue_tail = (queue->queue_tail + 1) % queue->queue_capacity;
    queue->queue_size++;
    
    printf("📥 NPU Queue: Added task %s (ID: %s) - Queue size: %d/%d\n", 
           task->method, task->request_id, queue->queue_size, queue->queue_capacity);
    
    // Signal worker thread
    pthread_cond_signal(&queue->queue_cond);
    pthread_mutex_unlock(&queue->queue_mutex);
    
    return 0;
}

bool npu_queue_is_busy(npu_queue_t* queue) {
    if (!queue) {
        return false;
    }
    
    pthread_mutex_lock(&queue->queue_mutex);
    bool busy = queue->npu_busy;
    pthread_mutex_unlock(&queue->queue_mutex);
    
    return busy;
}

bool npu_queue_is_full(npu_queue_t* queue) {
    if (!queue) {
        return true;
    }
    
    pthread_mutex_lock(&queue->queue_mutex);
    bool full = (queue->queue_size >= queue->queue_capacity);
    pthread_mutex_unlock(&queue->queue_mutex);
    
    return full;
}

int npu_queue_get_pending_count(npu_queue_t* queue) {
    if (!queue) {
        return 0;
    }
    
    pthread_mutex_lock(&queue->queue_mutex);
    int count = queue->queue_size;
    pthread_mutex_unlock(&queue->queue_mutex);
    
    return count;
}

const char* npu_queue_get_current_operation(npu_queue_t* queue) {
    if (!queue) {
        return "N/A";
    }
    
    pthread_mutex_lock(&queue->queue_mutex);
    static char operation_copy[NPU_OPERATION_NAME_SIZE];
    strncpy(operation_copy, queue->current_operation, NPU_OPERATION_NAME_SIZE - 1);
    operation_copy[NPU_OPERATION_NAME_SIZE - 1] = '\0';
    pthread_mutex_unlock(&queue->queue_mutex);
    
    return operation_copy;
}

void npu_queue_shutdown(npu_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    printf("🛑 NPU Queue: Shutdown requested\n");
    
    pthread_mutex_lock(&queue->queue_mutex);
    queue->shutdown_requested = true;
    queue->running = false;
    pthread_cond_signal(&queue->queue_cond);
    pthread_mutex_unlock(&queue->queue_mutex);
    
    // Wait for worker thread to finish
    if (pthread_join(queue->worker_thread, NULL) != 0) {
        printf("⚠️  NPU Queue: Failed to join worker thread\n");
    }
    
    printf("✅ NPU Queue: Worker thread stopped\n");
}

void npu_queue_cleanup(npu_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    npu_queue_print_stats(queue);
    
    // Clean up remaining tasks
    pthread_mutex_lock(&queue->queue_mutex);
    
    for (int i = 0; i < queue->queue_size; i++) {
        int index = (queue->queue_head + i) % queue->queue_capacity;
        npu_task_t* task = &queue->task_queue[index];
        free(task->method);
        free(task->params_json);
    }
    
    pthread_mutex_unlock(&queue->queue_mutex);
    
    // Destroy synchronization primitives
    pthread_cond_destroy(&queue->shutdown_cond);
    pthread_cond_destroy(&queue->queue_cond);
    pthread_mutex_destroy(&queue->queue_mutex);
    
    // Free queue memory
    free(queue->task_queue);
    
    printf("✅ NPU Queue: Cleanup completed\n");
}

void npu_queue_print_stats(npu_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    pthread_mutex_lock(&queue->queue_mutex);
    
    printf("📊 NPU Queue Statistics:\n");
    printf("   Tasks Processed: %d\n", queue->tasks_processed);
    printf("   Tasks Failed: %d\n", queue->tasks_failed);
    printf("   Queue Overflows: %d\n", queue->queue_overflows);
    printf("   Current Queue Size: %d/%d\n", queue->queue_size, queue->queue_capacity);
    printf("   NPU Currently Busy: %s\n", queue->npu_busy ? "YES" : "NO");
    
    if (queue->npu_busy) {
        time_t elapsed = time(NULL) - queue->operation_started_at;
        printf("   Current Operation: %s (ID: %s, Running: %ld seconds)\n", 
               queue->current_operation, queue->current_request_id, elapsed);
    }
    
    pthread_mutex_unlock(&queue->queue_mutex);
}