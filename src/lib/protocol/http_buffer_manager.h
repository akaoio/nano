#ifndef HTTP_BUFFER_MANAGER_H
#define HTTP_BUFFER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include "../core/stream_manager.h"

// Forward declaration - mcp_stream_chunk_t is defined in adapter.h
struct mcp_stream_chunk_t;

// Configuration constants
#define HTTP_MAX_BUFFERS 100
#define HTTP_INITIAL_CHUNK_CAPACITY 4096
#define HTTP_MAX_CHUNK_SIZE 65536
#define HTTP_REQUEST_ID_SIZE 64
#define HTTP_CLEANUP_INTERVAL_SECONDS 30
#define HTTP_BUFFER_TIMEOUT_SECONDS 60

// HTTP buffer structure for storing streaming chunks
typedef struct {
    char request_id[HTTP_REQUEST_ID_SIZE];
    char* chunks;
    size_t chunks_size;
    size_t chunks_capacity;
    uint32_t chunk_count;
    uint64_t created_timestamp;
    uint64_t last_access;
    bool completed;
    bool in_use;
} http_buffer_t;

// HTTP buffer manager structure
typedef struct {
    http_buffer_t* buffers;
    size_t buffer_count;
    size_t max_buffers;
    pthread_mutex_t manager_mutex;
    pthread_t cleanup_thread;
    bool initialized;
    bool running;
    uint32_t cleanup_interval_seconds;
    uint32_t buffer_timeout_seconds;
} http_buffer_manager_t;

// Function declarations using void pointers to avoid forward declaration issues
int http_buffer_manager_init(void* manager);
void http_buffer_manager_shutdown(void* manager);

void* http_buffer_manager_create_buffer(void* manager, const char* request_id);
void* http_buffer_manager_find_buffer(void* manager, const char* request_id);

int http_buffer_manager_add_chunk(void* manager, const char* request_id, const void* chunk);
int http_buffer_manager_get_chunks(void* manager, const char* request_id, char* output, size_t output_size, bool clear_after_read);

int http_buffer_manager_remove_buffer(void* manager, const char* request_id);
int http_buffer_manager_cleanup_expired(void* manager);

// Getter functions for status information
int http_buffer_manager_get_buffer_count(void* manager);
int http_buffer_manager_get_max_buffers(void* manager);
bool http_buffer_manager_is_initialized(void* manager);
int http_buffer_manager_get_cleanup_interval(void* manager);
int http_buffer_manager_get_timeout_seconds(void* manager);
int http_buffer_manager_get_buffer_size(void* manager, const char* request_id);
int http_buffer_manager_expire_buffer_for_testing(void* manager, const char* request_id, int timeout_seconds);

// Utility functions
uint64_t get_timestamp_ms(void);
static void* http_buffer_cleanup_thread(void* arg);

#endif // HTTP_BUFFER_MANAGER_H