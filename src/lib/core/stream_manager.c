#include "stream_manager.h"
#include "../protocol/adapter.h"
#include "../transport/manager.h"
#include "../../common/time_utils/time_utils.h"
#include <stdio.h>

// Global transport manager reference for streaming
extern transport_manager_t* get_global_transport_manager(void);
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

// Global stream manager instance
static stream_manager_t g_stream_manager = {0};

// Default configuration
#define DEFAULT_MAX_SESSIONS 64
#define SESSION_TIMEOUT_MS 30000  // 30 seconds
#define CLEANUP_INTERVAL_SECONDS 10

// Cleanup thread function
static void* stream_cleanup_thread(void* arg) {
    stream_manager_t* manager = (stream_manager_t*)arg;
    
    while (manager->running) {
        sleep(CLEANUP_INTERVAL_SECONDS);
        
        if (manager->running) {
            int cleaned = stream_manager_cleanup_expired_sessions();
            if (cleaned > 0) {
                printf("[STREAM_MANAGER] Cleaned up %d expired sessions\n", cleaned);
            }
        }
    }
    
    return NULL;
}

int stream_manager_init(stream_manager_t* manager) {
    if (!manager) {
        return -1;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&manager->manager_mutex, NULL) != 0) {
        return -1;
    }
    
    // Allocate sessions array
    manager->max_sessions = DEFAULT_MAX_SESSIONS;
    manager->sessions = calloc(manager->max_sessions, sizeof(stream_manager_session_t));
    if (!manager->sessions) {
        pthread_mutex_destroy(&manager->manager_mutex);
        return -1;
    }
    
    // Initialize session mutexes
    for (size_t i = 0; i < manager->max_sessions; i++) {
        if (pthread_mutex_init(&manager->sessions[i].session_mutex, NULL) != 0) {
            // Clean up already initialized mutexes
            for (size_t j = 0; j < i; j++) {
                pthread_mutex_destroy(&manager->sessions[j].session_mutex);
            }
            free(manager->sessions);
            pthread_mutex_destroy(&manager->manager_mutex);
            return -1;
        }
    }
    
    manager->session_count = 0;
    manager->initialized = true;
    manager->running = true;
    
    // Start cleanup thread
    if (pthread_create(&manager->cleanup_thread, NULL, stream_cleanup_thread, manager) != 0) {
        // Clean up on thread creation failure
        for (size_t i = 0; i < manager->max_sessions; i++) {
            pthread_mutex_destroy(&manager->sessions[i].session_mutex);
        }
        free(manager->sessions);
        pthread_mutex_destroy(&manager->manager_mutex);
        manager->initialized = false;
        manager->running = false;
        return -1;
    }
    
    printf("[STREAM_MANAGER] Initialized with %zu max sessions\n", manager->max_sessions);
    return 0;
}

void stream_manager_shutdown(stream_manager_t* manager) {
    if (!manager || !manager->initialized) {
        return;
    }
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    // Stop cleanup thread
    manager->running = false;
    pthread_mutex_unlock(&manager->manager_mutex);
    
    // Wait for cleanup thread to finish
    pthread_join(manager->cleanup_thread, NULL);
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    // Clean up all active sessions
    for (size_t i = 0; i < manager->max_sessions; i++) {
        if (manager->sessions[i].active) {
            pthread_mutex_lock(&manager->sessions[i].session_mutex);
            manager->sessions[i].active = false;
            memset(manager->sessions[i].session_id, 0, sizeof(manager->sessions[i].session_id));
            memset(manager->sessions[i].request_id, 0, sizeof(manager->sessions[i].request_id));
            pthread_mutex_unlock(&manager->sessions[i].session_mutex);
        }
        pthread_mutex_destroy(&manager->sessions[i].session_mutex);
    }
    
    free(manager->sessions);
    manager->sessions = NULL;
    manager->session_count = 0;
    manager->initialized = false;
    
    pthread_mutex_unlock(&manager->manager_mutex);
    pthread_mutex_destroy(&manager->manager_mutex);
    
    printf("[STREAM_MANAGER] Shutdown complete\n");
}

stream_manager_session_t* stream_manager_create_session(const char* request_id, transport_type_t type) {
    if (!request_id) {
        return NULL;
    }
    
    stream_manager_t* manager = &g_stream_manager;
    if (!manager->initialized) {
        return NULL;
    }
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    // Find an available session slot
    stream_manager_session_t* session = NULL;
    for (size_t i = 0; i < manager->max_sessions; i++) {
        if (!manager->sessions[i].active) {
            session = &manager->sessions[i];
            break;
        }
    }
    
    if (!session) {
        pthread_mutex_unlock(&manager->manager_mutex);
        return NULL; // No available slots
    }
    
    pthread_mutex_lock(&session->session_mutex);
    
    // Initialize session
    generate_session_id(session->session_id, sizeof(session->session_id));
    strncpy(session->request_id, request_id, sizeof(session->request_id) - 1);
    session->request_id[sizeof(session->request_id) - 1] = '\0';
    
    session->transport_type = type;
    session->transport_handle = NULL; // Will be set by transport layer
    session->rkllm_context = NULL;   // Will be set by RKLLM layer
    session->active = true;
    session->sequence_number = 0;
    session->created_timestamp = get_timestamp_ms();
    session->last_activity = session->created_timestamp;
    
    manager->session_count++;
    
    pthread_mutex_unlock(&session->session_mutex);
    pthread_mutex_unlock(&manager->manager_mutex);
    
    printf("[STREAM_MANAGER] Created session: %s (request_id: %s, transport: %d)\n", 
           session->session_id, session->request_id, type);
    
    return session;
}

stream_manager_session_t* stream_manager_get_session(const char* session_id) {
    if (!session_id) {
        return NULL;
    }
    
    stream_manager_t* manager = &g_stream_manager;
    if (!manager->initialized) {
        return NULL;
    }
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    for (size_t i = 0; i < manager->max_sessions; i++) {
        if (manager->sessions[i].active && 
            strcmp(manager->sessions[i].session_id, session_id) == 0) {
            pthread_mutex_unlock(&manager->manager_mutex);
            return &manager->sessions[i];
        }
    }
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return NULL;
}

int stream_manager_destroy_session(const char* session_id) {
    if (!session_id) {
        return -1;
    }
    
    stream_manager_t* manager = &g_stream_manager;
    if (!manager->initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    for (size_t i = 0; i < manager->max_sessions; i++) {
        if (manager->sessions[i].active && 
            strcmp(manager->sessions[i].session_id, session_id) == 0) {
            
            pthread_mutex_lock(&manager->sessions[i].session_mutex);
            
            printf("[STREAM_MANAGER] Destroying session: %s\n", session_id);
            
            manager->sessions[i].active = false;
            memset(manager->sessions[i].session_id, 0, sizeof(manager->sessions[i].session_id));
            memset(manager->sessions[i].request_id, 0, sizeof(manager->sessions[i].request_id));
            manager->sessions[i].transport_handle = NULL;
            manager->sessions[i].rkllm_context = NULL;
            manager->sessions[i].sequence_number = 0;
            
            pthread_mutex_unlock(&manager->sessions[i].session_mutex);
            
            manager->session_count--;
            pthread_mutex_unlock(&manager->manager_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return -1; // Session not found
}

int stream_manager_cleanup_expired_sessions(void) {
    stream_manager_t* manager = &g_stream_manager;
    if (!manager->initialized) {
        return 0;
    }
    
    uint64_t current_time = get_timestamp_ms();
    int cleaned_count = 0;
    
    pthread_mutex_lock(&manager->manager_mutex);
    
    for (size_t i = 0; i < manager->max_sessions; i++) {
        if (manager->sessions[i].active) {
            pthread_mutex_lock(&manager->sessions[i].session_mutex);
            
            uint64_t inactive_time = current_time - manager->sessions[i].last_activity;
            if (inactive_time > SESSION_TIMEOUT_MS) {
                printf("[STREAM_MANAGER] Cleaning up expired session: %s (inactive for %lu ms)\n",
                       manager->sessions[i].session_id, inactive_time);
                
                manager->sessions[i].active = false;
                memset(manager->sessions[i].session_id, 0, sizeof(manager->sessions[i].session_id));
                memset(manager->sessions[i].request_id, 0, sizeof(manager->sessions[i].request_id));
                manager->sessions[i].transport_handle = NULL;
                manager->sessions[i].rkllm_context = NULL;
                
                manager->session_count--;
                cleaned_count++;
            }
            
            pthread_mutex_unlock(&manager->sessions[i].session_mutex);
        }
    }
    
    pthread_mutex_unlock(&manager->manager_mutex);
    return cleaned_count;
}

void generate_session_id(char* session_id, size_t buffer_size) {
    if (!session_id || buffer_size < 32) {
        return;
    }
    
    uint64_t timestamp = get_timestamp_ms();
    int random_part = rand();
    
    snprintf(session_id, buffer_size, "stream_%llu_%08x", 
             (unsigned long long)timestamp, random_part);
}

// get_timestamp_ms is now defined in common/time_utils/time_utils.c

stream_manager_t* get_global_stream_manager(void) {
    return &g_stream_manager;
}

// Placeholder for transport integration - will be implemented by transport layer
int transport_send_stream_chunk(void* transport_handle, void* chunk_ptr) {
    mcp_stream_chunk_t* chunk = (mcp_stream_chunk_t*)chunk_ptr;
    if (!chunk) {
        return -1;
    }
    
    // Get global transport manager
    transport_manager_t* manager = get_global_transport_manager();
    if (!manager) {
        printf("[TRANSPORT] Error: No transport manager available\n");
        return -1;
    }
    
    // Send chunk through transport manager to all active transports
    int result = transport_manager_send_stream_chunk(manager, chunk);
    
    printf("[TRANSPORT] Stream chunk: request_id=%s, seq=%u, delta='%.50s', end=%s, sent=%s\n",
           chunk->request_id, chunk->seq, chunk->delta, chunk->end ? "true" : "false",
           result == 0 ? "success" : "failed");
    
    if (chunk->error_message) {
        printf("[TRANSPORT] Error: %s\n", chunk->error_message);
    }
    
    return result;
}