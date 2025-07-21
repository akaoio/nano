#ifndef ASYNC_RESPONSE_H
#define ASYNC_RESPONSE_H

#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#define ASYNC_RESPONSE_REQUEST_ID_SIZE 64
#define ASYNC_RESPONSE_MAX_RESULT_SIZE 8192
#define ASYNC_RESPONSE_DEFAULT_CAPACITY 100

typedef struct {
    char request_id[ASYNC_RESPONSE_REQUEST_ID_SIZE];
    int transport_index;
    void* connection_handle;
    char* result_json;
    size_t result_size;
    bool completed;
    bool error;
    time_t started_at;
    time_t completed_at;
    time_t expires_at;  // For cleanup
} async_response_t;

// Response registry for async operations
typedef struct {
    async_response_t* responses;
    int capacity;
    int count;
    pthread_mutex_t mutex;
    time_t last_cleanup;
} async_response_registry_t;

// Global response registry
extern async_response_registry_t* g_response_registry;

// Function declarations
int async_response_registry_init(async_response_registry_t* registry, int capacity);
int async_response_registry_add(async_response_registry_t* registry, const async_response_t* response);
async_response_t* async_response_registry_find(async_response_registry_t* registry, const char* request_id);
int async_response_registry_remove(async_response_registry_t* registry, const char* request_id);
void async_response_registry_cleanup_expired(async_response_registry_t* registry);
void async_response_registry_shutdown(async_response_registry_t* registry);
void async_response_registry_print_stats(async_response_registry_t* registry);

// Helper functions
async_response_t* async_response_create(const char* request_id, int transport_index, void* connection_handle);
void async_response_set_result(async_response_t* response, const char* result_json, bool is_error);
void async_response_free(async_response_t* response);

#endif // ASYNC_RESPONSE_H