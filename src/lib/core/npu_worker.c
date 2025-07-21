#include "npu_queue.h"
#include "async_response.h"
#include "operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void* npu_worker_thread(void* arg) {
    npu_queue_t* queue = (npu_queue_t*)arg;
    
    printf("🚀 NPU Worker: Thread started\n");
    
    while (queue->running) {
        pthread_mutex_lock(&queue->queue_mutex);
        
        // Wait for NPU tasks
        while (queue->queue_head == queue->queue_tail && queue->running) {
            pthread_cond_wait(&queue->queue_cond, &queue->queue_mutex);
        }
        
        if (!queue->running) {
            pthread_mutex_unlock(&queue->queue_mutex);
            break;
        }
        
        // Get NPU task from queue
        npu_task_t task = queue->task_queue[queue->queue_head];
        queue->queue_head = (queue->queue_head + 1) % queue->queue_capacity;
        queue->queue_size--;
        
        // Mark NPU as busy
        queue->npu_busy = true;
        strncpy(queue->current_operation, task.method, sizeof(queue->current_operation) - 1);
        queue->current_operation[sizeof(queue->current_operation) - 1] = '\0';
        strncpy(queue->current_request_id, task.request_id, sizeof(queue->current_request_id) - 1);
        queue->current_request_id[sizeof(queue->current_request_id) - 1] = '\0';
        queue->operation_started_at = time(NULL);
        
        pthread_mutex_unlock(&queue->queue_mutex);
        
        printf("🔄 NPU Worker: Starting %s (Request ID: %s)\n", task.method, task.request_id);
        
        // Process NPU task (SINGLE INSTANCE ONLY - Hardware constraint)
        char* result_json = NULL;
        int status = io_process_operation(task.method, task.params_json, &result_json);
        
        time_t processing_time = time(NULL) - queue->operation_started_at;
        
        if (status == 0) {
            printf("✅ NPU Worker: Completed %s in %ld seconds (Request ID: %s)\n", 
                   task.method, processing_time, task.request_id);
            queue->tasks_processed++;
        } else {
            printf("❌ NPU Worker: Failed %s after %ld seconds (Request ID: %s)\n", 
                   task.method, processing_time, task.request_id);
            queue->tasks_failed++;
        }
        
        // Create async response
        async_response_t response = {
            .completed = true,
            .error = (status != 0),
            .result_json = result_json,  // Will be deep copied by registry
            .started_at = queue->operation_started_at,
            .completed_at = time(NULL),
            .transport_index = task.transport_index,
            .connection_handle = task.connection_handle
        };
        strncpy(response.request_id, task.request_id, ASYNC_RESPONSE_REQUEST_ID_SIZE - 1);
        response.request_id[ASYNC_RESPONSE_REQUEST_ID_SIZE - 1] = '\0';
        
        // Store result in response registry
        if (g_response_registry) {
            if (async_response_registry_add(g_response_registry, &response) != 0) {
                printf("⚠️  NPU Worker: Failed to store response for %s (Request ID: %s)\n", 
                       task.method, task.request_id);
            }
        } else {
            printf("⚠️  NPU Worker: No response registry available for %s (Request ID: %s)\n", 
                   task.method, task.request_id);
        }
        
        // Mark NPU as free
        pthread_mutex_lock(&queue->queue_mutex);
        queue->npu_busy = false;
        memset(queue->current_operation, 0, sizeof(queue->current_operation));
        memset(queue->current_request_id, 0, sizeof(queue->current_request_id));
        pthread_mutex_unlock(&queue->queue_mutex);
        
        // Cleanup task memory
        free(task.method);
        free(task.params_json);
        
        // Free result_json as it was deep copied by the response registry
        if (result_json) {
            free(result_json);
        }
        
        printf("🆓 NPU Worker: NPU is now available for next operation\n");
    }
    
    printf("🛑 NPU Worker: Thread stopping\n");
    
    // Signal shutdown completion
    pthread_mutex_lock(&queue->queue_mutex);
    pthread_cond_signal(&queue->shutdown_cond);
    pthread_mutex_unlock(&queue->queue_mutex);
    
    return NULL;
}