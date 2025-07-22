#define _DEFAULT_SOURCE
#include "http_buffer_manager.h"
#include "../../common/time_utils/time_utils.h"

// Define the actual struct since we need to access members
struct mcp_stream_chunk_t {
    char request_id[32];
    char method[64];
    uint32_t seq;
    char delta[2048];
    bool end;
    char* error_message;
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

// get_timestamp_ms is included from time_utils.h

// Initialize HTTP buffer manager
int http_buffer_manager_init(void* manager_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager) return -1;
    
    memset(manager, 0, sizeof(http_buffer_manager_t));
    
    // Allocate buffer array
    manager->max_buffers = HTTP_MAX_BUFFERS;
    manager->buffers = calloc(manager->max_buffers, sizeof(http_buffer_t));
    if (!manager->buffers) {
        return -1;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&manager->manager_mutex, NULL) != 0) {
        free(manager->buffers);
        return -1;
    }
    
    // Set configuration
    manager->cleanup_interval_seconds = HTTP_CLEANUP_INTERVAL_SECONDS;
    manager->buffer_timeout_seconds = HTTP_BUFFER_TIMEOUT_SECONDS;
    manager->running = true;
    
    // Start cleanup thread
    if (pthread_create(&manager->cleanup_thread, NULL, http_buffer_cleanup_thread, manager) != 0) {
        pthread_mutex_destroy(&manager->manager_mutex);
        free(manager->buffers);
        return -1;
    }
    
    manager->initialized = true;
    printf("HTTP Buffer Manager: Initialized with %d max buffers\n", HTTP_MAX_BUFFERS);
    return 0;
}

// Shutdown HTTP buffer manager
void http_buffer_manager_shutdown(void* manager_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager || !manager->initialized) return;
    
    printf("HTTP Buffer Manager: Shutting down...\n");
    manager->running = false;
    
    // Wait for cleanup thread to finish
    pthread_join(manager->cleanup_thread, NULL);
    
    // Free all buffers
    pthread_mutex_lock(&manager->manager_mutex);
    for (size_t i = 0; i < manager->buffer_count; i++) {
        if (manager->buffers[i].chunks) {
            free(manager->buffers[i].chunks);
        }
    }
    free(manager->buffers);
    pthread_mutex_unlock(&manager->manager_mutex);
    
    pthread_mutex_destroy(&manager->manager_mutex);
    manager->initialized = false;
    printf("HTTP Buffer Manager: Shutdown complete\n");
}

// Create new buffer for request ID
void* http_buffer_manager_create_buffer(void* manager_ptr, const char* request_id) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager || !request_id) return NULL;
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    // Check if buffer already exists
    for (size_t i = 0; i < manager->buffer_count; i++) {
        if (manager->buffers[i].in_use && strcmp(manager->buffers[i].request_id, request_id) == 0) {
            manager->buffers[i].last_access = get_timestamp_ms();
            pthread_mutex_unlock(&manager->manager_mutex);
            return &manager->buffers[i]; // Return existing buffer
        }
    }
    
    // Find empty slot or create new one
    http_buffer_t* buffer = NULL;
    size_t buffer_index = 0;
    
    // Look for unused slot first
    for (size_t i = 0; i < manager->buffer_count; i++) {
        if (!manager->buffers[i].in_use) {
            buffer = &manager->buffers[i];
            buffer_index = i;
            break;
        }
    }
    
    // If no unused slot, create new one if space available
    if (!buffer && manager->buffer_count < manager->max_buffers) {
        buffer = &manager->buffers[manager->buffer_count];
        buffer_index = manager->buffer_count;
        manager->buffer_count++;
    }
    
    if (!buffer) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return NULL; // No space available
    }
    
    // Initialize buffer
    memset(buffer, 0, sizeof(http_buffer_t));
    strncpy(buffer->request_id, request_id, sizeof(buffer->request_id) - 1);
    buffer->chunks_capacity = HTTP_INITIAL_CHUNK_CAPACITY;
    buffer->chunks = malloc(buffer->chunks_capacity);
    buffer->created_timestamp = get_timestamp_ms();
    buffer->last_access = buffer->created_timestamp;
    buffer->in_use = true;
    
    if (!buffer->chunks) {
        buffer->in_use = false;
        pthread_mutex_unlock(&manager->manager_mutex);
        return NULL;
    }
    
    buffer->chunks[0] = '\0';
    
    printf("HTTP Buffer Manager: Created buffer for request_id '%s' (index %zu)\n", request_id, buffer_index);
    pthread_mutex_unlock(&manager->manager_mutex);
    return buffer;
}

// Find existing buffer by request ID
void* http_buffer_manager_find_buffer(void* manager_ptr, const char* request_id) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager || !request_id) return NULL;
    
    pthread_mutex_lock(&manager->manager_mutex);
    for (size_t i = 0; i < manager->buffer_count; i++) {
        if (manager->buffers[i].in_use && strcmp(manager->buffers[i].request_id, request_id) == 0) {
            manager->buffers[i].last_access = get_timestamp_ms();
            pthread_mutex_unlock(&manager->manager_mutex);
            return &manager->buffers[i];
        }
    }
    pthread_mutex_unlock(&manager->manager_mutex);
    return NULL;
}

// Add chunk to buffer
int http_buffer_manager_add_chunk(void* manager_ptr, const char* request_id, const void* chunk_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    const struct mcp_stream_chunk_t* chunk = (const struct mcp_stream_chunk_t*)chunk_ptr;
    if (!manager || !request_id || !chunk) return -1;
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    // Find buffer
    http_buffer_t* buffer = NULL;
    for (size_t i = 0; i < manager->buffer_count; i++) {
        if (manager->buffers[i].in_use && strcmp(manager->buffers[i].request_id, request_id) == 0) {
            buffer = &manager->buffers[i];
            break;
        }
    }
    
    if (!buffer) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return -1; // Buffer not found
    }
    
    // Format chunk as JSON
    char chunk_json[1024];
    snprintf(chunk_json, sizeof(chunk_json),
             "{\"seq\":%u,\"delta\":\"%s\",\"end\":%s}",
             chunk->seq, chunk->delta ? chunk->delta : "", chunk->end ? "true" : "false");
    
    size_t chunk_len = strlen(chunk_json);
    size_t current_len = strlen(buffer->chunks);
    
    // Expand buffer if needed
    if (current_len + chunk_len + 2 > buffer->chunks_capacity) {
        size_t new_capacity = buffer->chunks_capacity * 2;
        while (new_capacity < current_len + chunk_len + 2) {
            new_capacity *= 2;
        }
        if (new_capacity > HTTP_MAX_CHUNK_SIZE) {
            new_capacity = HTTP_MAX_CHUNK_SIZE;
        }
        
        char* new_chunks = realloc(buffer->chunks, new_capacity);
        if (!new_chunks) {
            pthread_mutex_unlock(&manager->manager_mutex);
            return -1;
        }
        buffer->chunks = new_chunks;
        buffer->chunks_capacity = new_capacity;
    }
    
    // Add chunk to buffer (concatenate with separator)
    if (current_len > 0) {
        strcat(buffer->chunks, ",");
    }
    strcat(buffer->chunks, chunk_json);
    
    buffer->chunk_count++;
    buffer->last_access = get_timestamp_ms();
    
    if (chunk->end) {
        buffer->completed = true;
    }
    
    printf("HTTP Buffer Manager: Added chunk %u to request_id '%s' (total chunks: %u)\n", 
           chunk->seq, request_id, buffer->chunk_count);
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return 0;
}

// Get chunks from buffer
int http_buffer_manager_get_chunks(void* manager_ptr, const char* request_id, char* output, size_t output_size, bool clear_after_read) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager || !request_id || !output) return -1;
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    // Find buffer
    http_buffer_t* buffer = NULL;
    size_t buffer_index = 0;
    for (size_t i = 0; i < manager->buffer_count; i++) {
        if (manager->buffers[i].in_use && strcmp(manager->buffers[i].request_id, request_id) == 0) {
            buffer = &manager->buffers[i];
            buffer_index = i;
            break;
        }
    }
    
    if (!buffer) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return -1; // Buffer not found
    }
    
    // Copy chunks to output
    size_t chunks_len = strlen(buffer->chunks);
    if (chunks_len >= output_size) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return -1; // Output buffer too small
    }
    
    strcpy(output, buffer->chunks);
    buffer->last_access = get_timestamp_ms();
    
    bool should_clear = clear_after_read || buffer->completed;
    
    printf("HTTP Buffer Manager: Retrieved %zu bytes for request_id '%s' (clear: %s)\n", 
           chunks_len, request_id, should_clear ? "yes" : "no");
    
    // Clear buffer if requested or if completed
    if (should_clear) {
        free(buffer->chunks);
        buffer->chunks = NULL;
        buffer->in_use = false;
        memset(buffer->request_id, 0, sizeof(buffer->request_id));
        printf("HTTP Buffer Manager: Cleared buffer for request_id '%s'\n", request_id);
    }
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return 0;
}

// Remove buffer by request ID
int http_buffer_manager_remove_buffer(void* manager_ptr, const char* request_id) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager || !request_id) return -1;
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    for (size_t i = 0; i < manager->buffer_count; i++) {
        if (manager->buffers[i].in_use && strcmp(manager->buffers[i].request_id, request_id) == 0) {
            if (manager->buffers[i].chunks) {
                free(manager->buffers[i].chunks);
            }
            manager->buffers[i].in_use = false;
            memset(&manager->buffers[i], 0, sizeof(http_buffer_t));
            printf("HTTP Buffer Manager: Removed buffer for request_id '%s'\n", request_id);
            pthread_mutex_unlock(&manager->manager_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return -1; // Buffer not found
}

// Clean up expired buffers
int http_buffer_manager_cleanup_expired(void* manager_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager) return -1;
    
    uint64_t current_time = get_timestamp_ms();
    uint64_t timeout_ms = manager->buffer_timeout_seconds * 1000;
    int cleaned = 0;
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    for (size_t i = 0; i < manager->buffer_count; i++) {
        if (manager->buffers[i].in_use && 
            (current_time - manager->buffers[i].last_access) > timeout_ms) {
            
            printf("HTTP Buffer Manager: Cleaning expired buffer for request_id '%s' (age: %llu ms)\n",
                   manager->buffers[i].request_id, current_time - manager->buffers[i].last_access);
            
            if (manager->buffers[i].chunks) {
                free(manager->buffers[i].chunks);
            }
            manager->buffers[i].in_use = false;
            memset(&manager->buffers[i], 0, sizeof(http_buffer_t));
            cleaned++;
        }
    }
    
    pthread_mutex_unlock(&manager->manager_mutex);
    
    if (cleaned > 0) {
        printf("HTTP Buffer Manager: Cleaned up %d expired buffers\n", cleaned);
    }
    
    return cleaned;
}

// Cleanup thread function
static void* http_buffer_cleanup_thread(void* arg) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)arg;
    
    printf("HTTP Buffer Manager: Cleanup thread started\n");
    
    while (manager->running) {
        sleep(manager->cleanup_interval_seconds);
        
        if (manager->running) {
            http_buffer_manager_cleanup_expired(manager);
        }
    }
    
    printf("HTTP Buffer Manager: Cleanup thread exiting\n");
    return NULL;
}

// Getter functions for status information
int http_buffer_manager_get_buffer_count(void* manager_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager) return -1;
    
    pthread_mutex_lock(&manager->manager_mutex);
    int count = 0;
    for (size_t i = 0; i < manager->max_buffers; i++) {
        if (manager->buffers[i].in_use) {
            count++;
        }
    }
    pthread_mutex_unlock(&manager->manager_mutex);
    return count;
}

int http_buffer_manager_get_max_buffers(void* manager_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    return manager ? (int)manager->max_buffers : -1;
}

bool http_buffer_manager_is_initialized(void* manager_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    return manager && manager->initialized;
}

int http_buffer_manager_get_cleanup_interval(void* manager_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    return manager ? (int)manager->cleanup_interval_seconds : -1;
}

int http_buffer_manager_get_timeout_seconds(void* manager_ptr) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    return manager ? (int)manager->buffer_timeout_seconds : -1;
}

int http_buffer_manager_get_buffer_size(void* manager_ptr, const char* request_id) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager || !request_id) return -1;
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    for (size_t i = 0; i < manager->max_buffers; i++) {
        if (manager->buffers[i].in_use && 
            strcmp(manager->buffers[i].request_id, request_id) == 0) {
            int size = manager->buffers[i].chunks ? (int)strlen(manager->buffers[i].chunks) : 0;
            pthread_mutex_unlock(&manager->manager_mutex);
            return size;
        }
    }
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return -1; // Buffer not found
}

int http_buffer_manager_expire_buffer_for_testing(void* manager_ptr, const char* request_id, int timeout_seconds) {
    http_buffer_manager_t* manager = (http_buffer_manager_t*)manager_ptr;
    if (!manager || !request_id) return -1;
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    for (size_t i = 0; i < manager->max_buffers; i++) {
        if (manager->buffers[i].in_use && 
            strcmp(manager->buffers[i].request_id, request_id) == 0) {
            // Set old timestamp to simulate expiration
            manager->buffers[i].last_access = get_timestamp_ms() - (timeout_seconds + 1) * 1000;
            pthread_mutex_unlock(&manager->manager_mutex);
            return 0; // Success
        }
    }
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return -1; // Buffer not found
}