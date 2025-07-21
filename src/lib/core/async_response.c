#include "async_response.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define RESPONSE_EXPIRY_SECONDS 300  // 5 minutes
#define CLEANUP_INTERVAL_SECONDS 60  // 1 minute

// Global response registry
async_response_registry_t* g_response_registry = NULL;

int async_response_registry_init(async_response_registry_t* registry, int capacity) {
    if (!registry) {
        return -1;
    }
    
    memset(registry, 0, sizeof(async_response_registry_t));
    
    registry->responses = malloc(capacity * sizeof(async_response_t));
    if (!registry->responses) {
        printf("❌ Async Response Registry: Failed to allocate responses array\n");
        return -1;
    }
    
    registry->capacity = capacity;
    registry->count = 0;
    registry->last_cleanup = time(NULL);
    
    if (pthread_mutex_init(&registry->mutex, NULL) != 0) {
        printf("❌ Async Response Registry: Failed to initialize mutex\n");
        free(registry->responses);
        return -1;
    }
    
    printf("✅ Async Response Registry: Initialized with capacity %d\n", capacity);
    return 0;
}

int async_response_registry_add(async_response_registry_t* registry, const async_response_t* response) {
    if (!registry || !response) {
        return -1;
    }
    
    pthread_mutex_lock(&registry->mutex);
    
    // Check capacity
    if (registry->count >= registry->capacity) {
        pthread_mutex_unlock(&registry->mutex);
        printf("⚠️  Async Response Registry: Registry full, cannot add response for ID: %s\n", 
               response->request_id);
        return -2; // Registry full
    }
    
    // Find empty slot or update existing
    int slot = -1;
    for (int i = 0; i < registry->capacity; i++) {
        if (registry->responses[i].request_id[0] == '\0') {
            slot = i;
            break;
        } else if (strcmp(registry->responses[i].request_id, response->request_id) == 0) {
            // Update existing entry
            slot = i;
            // Free old result if exists
            if (registry->responses[i].result_json) {
                free(registry->responses[i].result_json);
            }
            break;
        }
    }
    
    if (slot == -1) {
        pthread_mutex_unlock(&registry->mutex);
        printf("❌ Async Response Registry: No available slot found\n");
        return -3; // No slot available
    }
    
    // Copy response data
    async_response_t* target = &registry->responses[slot];
    strncpy(target->request_id, response->request_id, ASYNC_RESPONSE_REQUEST_ID_SIZE - 1);
    target->request_id[ASYNC_RESPONSE_REQUEST_ID_SIZE - 1] = '\0';
    target->transport_index = response->transport_index;
    target->connection_handle = response->connection_handle;
    target->completed = response->completed;
    target->error = response->error;
    target->started_at = response->started_at;
    target->completed_at = response->completed_at;
    target->expires_at = time(NULL) + RESPONSE_EXPIRY_SECONDS;
    
    // Deep copy result JSON
    if (response->result_json) {
        target->result_json = strdup(response->result_json);
        target->result_size = strlen(response->result_json);
    } else {
        target->result_json = NULL;
        target->result_size = 0;
    }
    
    // Update count if this is a new entry
    bool was_empty = (target->request_id[0] == '\0');
    if (was_empty) {
        registry->count++;
    }
    
    printf("📝 Async Response Registry: Added response for ID: %s (Slot: %d, Total: %d)\n", 
           response->request_id, slot, registry->count);
    
    pthread_mutex_unlock(&registry->mutex);
    return 0;
}

async_response_t* async_response_registry_find(async_response_registry_t* registry, const char* request_id) {
    if (!registry || !request_id) {
        return NULL;
    }
    
    pthread_mutex_lock(&registry->mutex);
    
    // Periodic cleanup
    time_t now = time(NULL);
    if (now - registry->last_cleanup > CLEANUP_INTERVAL_SECONDS) {
        async_response_registry_cleanup_expired(registry);
        registry->last_cleanup = now;
    }
    
    for (int i = 0; i < registry->capacity; i++) {
        if (strcmp(registry->responses[i].request_id, request_id) == 0) {
            async_response_t* response = &registry->responses[i];
            pthread_mutex_unlock(&registry->mutex);
            return response;
        }
    }
    
    pthread_mutex_unlock(&registry->mutex);
    return NULL;
}

int async_response_registry_remove(async_response_registry_t* registry, const char* request_id) {
    if (!registry || !request_id) {
        return -1;
    }
    
    pthread_mutex_lock(&registry->mutex);
    
    for (int i = 0; i < registry->capacity; i++) {
        if (strcmp(registry->responses[i].request_id, request_id) == 0) {
            async_response_t* response = &registry->responses[i];
            
            // Free allocated memory
            if (response->result_json) {
                free(response->result_json);
            }
            
            // Clear entry
            memset(response, 0, sizeof(async_response_t));
            registry->count--;
            
            printf("🗑️  Async Response Registry: Removed response for ID: %s (Total: %d)\n", 
                   request_id, registry->count);
            
            pthread_mutex_unlock(&registry->mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&registry->mutex);
    return -2; // Not found
}

void async_response_registry_cleanup_expired(async_response_registry_t* registry) {
    if (!registry) {
        return;
    }
    
    // Note: Caller must hold mutex
    time_t now = time(NULL);
    int cleaned = 0;
    
    for (int i = 0; i < registry->capacity; i++) {
        async_response_t* response = &registry->responses[i];
        if (response->request_id[0] != '\0' && now > response->expires_at) {
            // Free allocated memory
            if (response->result_json) {
                free(response->result_json);
            }
            
            // Clear entry
            memset(response, 0, sizeof(async_response_t));
            registry->count--;
            cleaned++;
        }
    }
    
    if (cleaned > 0) {
        printf("🧹 Async Response Registry: Cleaned up %d expired responses (Total: %d)\n", 
               cleaned, registry->count);
    }
}

void async_response_registry_shutdown(async_response_registry_t* registry) {
    if (!registry) {
        return;
    }
    
    printf("🛑 Async Response Registry: Shutdown requested\n");
    
    pthread_mutex_lock(&registry->mutex);
    
    // Clean up all responses
    for (int i = 0; i < registry->capacity; i++) {
        async_response_t* response = &registry->responses[i];
        if (response->result_json) {
            free(response->result_json);
        }
    }
    
    free(registry->responses);
    registry->responses = NULL;
    registry->count = 0;
    registry->capacity = 0;
    
    pthread_mutex_unlock(&registry->mutex);
    pthread_mutex_destroy(&registry->mutex);
    
    printf("✅ Async Response Registry: Shutdown completed\n");
}

void async_response_registry_print_stats(async_response_registry_t* registry) {
    if (!registry) {
        return;
    }
    
    pthread_mutex_lock(&registry->mutex);
    
    printf("📊 Async Response Registry Statistics:\n");
    printf("   Active Responses: %d/%d\n", registry->count, registry->capacity);
    
    int completed = 0, pending = 0, errors = 0;
    for (int i = 0; i < registry->capacity; i++) {
        async_response_t* response = &registry->responses[i];
        if (response->request_id[0] != '\0') {
            if (response->completed) {
                if (response->error) {
                    errors++;
                } else {
                    completed++;
                }
            } else {
                pending++;
            }
        }
    }
    
    printf("   Completed: %d, Pending: %d, Errors: %d\n", completed, pending, errors);
    
    pthread_mutex_unlock(&registry->mutex);
}

// Helper functions
async_response_t* async_response_create(const char* request_id, int transport_index, void* connection_handle) {
    async_response_t* response = malloc(sizeof(async_response_t));
    if (!response) {
        return NULL;
    }
    
    memset(response, 0, sizeof(async_response_t));
    strncpy(response->request_id, request_id, ASYNC_RESPONSE_REQUEST_ID_SIZE - 1);
    response->request_id[ASYNC_RESPONSE_REQUEST_ID_SIZE - 1] = '\0';
    response->transport_index = transport_index;
    response->connection_handle = connection_handle;
    response->started_at = time(NULL);
    response->completed = false;
    response->error = false;
    
    return response;
}

void async_response_set_result(async_response_t* response, const char* result_json, bool is_error) {
    if (!response) {
        return;
    }
    
    if (response->result_json) {
        free(response->result_json);
    }
    
    if (result_json) {
        response->result_json = strdup(result_json);
        response->result_size = strlen(result_json);
    } else {
        response->result_json = NULL;
        response->result_size = 0;
    }
    
    response->completed = true;
    response->error = is_error;
    response->completed_at = time(NULL);
}

void async_response_free(async_response_t* response) {
    if (!response) {
        return;
    }
    
    if (response->result_json) {
        free(response->result_json);
    }
    
    free(response);
}