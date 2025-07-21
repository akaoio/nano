#ifndef NPU_QUEUE_H
#define NPU_QUEUE_H

#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include "npu_operation_classifier.h"

#define NPU_QUEUE_MAX_SIZE 100
#define NPU_TASK_REQUEST_ID_SIZE 64
#define NPU_OPERATION_NAME_SIZE 64

typedef struct {
    char* method;
    char* params_json;
    char request_id[NPU_TASK_REQUEST_ID_SIZE];
    int transport_index;
    void* connection_handle;
    time_t queued_at;
    npu_operation_type_t op_type;
} npu_task_t;

typedef struct {
    pthread_t worker_thread;        // Single worker thread (NPU constraint)
    
    // Task queue for NPU operations only
    npu_task_t* task_queue;
    int queue_size;
    int queue_capacity;
    int queue_head, queue_tail;
    
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    pthread_cond_t shutdown_cond;
    
    // NPU status tracking
    volatile bool npu_busy;
    volatile bool running;
    volatile bool shutdown_requested;
    char current_operation[NPU_OPERATION_NAME_SIZE];
    char current_request_id[NPU_TASK_REQUEST_ID_SIZE];
    time_t operation_started_at;
    
    // Statistics
    int tasks_processed;
    int tasks_failed;
    int queue_overflows;
} npu_queue_t;

// Global NPU queue instance
extern npu_queue_t* g_npu_queue;

// Function declarations
int npu_queue_init(npu_queue_t* queue, int max_queue_size);
int npu_queue_add_task(npu_queue_t* queue, const npu_task_t* task);
bool npu_queue_is_busy(npu_queue_t* queue);
bool npu_queue_is_full(npu_queue_t* queue);
int npu_queue_get_pending_count(npu_queue_t* queue);
const char* npu_queue_get_current_operation(npu_queue_t* queue);
void npu_queue_shutdown(npu_queue_t* queue);
void npu_queue_cleanup(npu_queue_t* queue);

// Worker thread function
void* npu_worker_thread(void* arg);

// Statistics functions
void npu_queue_print_stats(npu_queue_t* queue);

#endif // NPU_QUEUE_H