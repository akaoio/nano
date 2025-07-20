#include "streaming.h"
#include "common/types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static stream_manager_t g_stream_manager = {0};

// Generate random stream ID (a-z0-9)
char* stream_generate_id(void) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    char* stream_id = malloc(STREAM_ID_LENGTH + 1);
    if (!stream_id) return NULL;
    
    srand((unsigned int)time(NULL) + rand());
    for (int i = 0; i < STREAM_ID_LENGTH; i++) {
        stream_id[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    stream_id[STREAM_ID_LENGTH] = '\0';
    
    return stream_id;
}

int stream_manager_init(void) {
    if (g_stream_manager.initialized) {
        return 0; // Already initialized
    }
    
    memset(&g_stream_manager, 0, sizeof(stream_manager_t));
    g_stream_manager.initialized = true;
    g_stream_manager.active_count = 0;
    
    return 0;
}

void stream_manager_shutdown(void) {
    if (!g_stream_manager.initialized) return;
    
    // Clean up all active sessions
    for (uint32_t i = 0; i < MAX_ACTIVE_STREAMS; i++) {
        if (g_stream_manager.sessions[i].initialized) {
            stream_destroy_session(g_stream_manager.sessions[i].stream_id);
        }
    }
    
    g_stream_manager.initialized = false;
    g_stream_manager.active_count = 0;
}

stream_session_t* stream_create_session(const char* method, uint32_t request_id) {
    if (!g_stream_manager.initialized || !method) return NULL;
    
    // Find empty slot
    stream_session_t* session = NULL;
    for (uint32_t i = 0; i < MAX_ACTIVE_STREAMS; i++) {
        if (!g_stream_manager.sessions[i].initialized) {
            session = &g_stream_manager.sessions[i];
            break;
        }
    }
    
    if (!session) return NULL; // No available slots
    
    // Generate unique stream ID
    char* stream_id = stream_generate_id();
    if (!stream_id) return NULL;
    
    // Initialize session
    memset(session, 0, sizeof(stream_session_t));
    strncpy(session->stream_id, stream_id, STREAM_ID_LENGTH);
    strncpy(session->original_method, method, sizeof(session->original_method) - 1);
    session->request_id = request_id;
    session->state = STREAM_STATE_INITIALIZING;
    session->next_seq = 0;
    session->last_consumed_seq = 0;
    session->created_time = time(NULL);
    session->last_access_time = session->created_time;
    session->auto_cleanup = true;
    session->initialized = true;
    
    g_stream_manager.active_count++;
    
    free(stream_id);
    return session;
}

stream_session_t* stream_get_session(const char* stream_id) {
    if (!g_stream_manager.initialized || !stream_id) return NULL;
    
    for (uint32_t i = 0; i < MAX_ACTIVE_STREAMS; i++) {
        stream_session_t* session = &g_stream_manager.sessions[i];
        if (session->initialized && strcmp(session->stream_id, stream_id) == 0) {
            session->last_access_time = time(NULL);
            return session;
        }
    }
    
    return NULL;
}

int stream_destroy_session(const char* stream_id) {
    if (!g_stream_manager.initialized || !stream_id) return -1;
    
    stream_session_t* session = stream_get_session(stream_id);
    if (!session) return -1;
    
    // Free all chunks
    stream_chunk_t* chunk = session->chunk_head;
    while (chunk) {
        stream_chunk_t* next = chunk->next;
        free(chunk->delta);
        free(chunk->error_message);
        free(chunk);
        chunk = next;
    }
    
    // Reset session
    memset(session, 0, sizeof(stream_session_t));
    g_stream_manager.active_count--;
    
    return 0;
}

bool stream_is_expired(const stream_session_t* session) {
    if (!session) return true;
    
    time_t now = time(NULL);
    return (now - session->last_access_time) > STREAM_SESSION_TIMEOUT_SEC;
}

void stream_cleanup_expired_sessions(void) {
    if (!g_stream_manager.initialized) return;
    
    for (uint32_t i = 0; i < MAX_ACTIVE_STREAMS; i++) {
        stream_session_t* session = &g_stream_manager.sessions[i];
        if (session->initialized && stream_is_expired(session)) {
            stream_destroy_session(session->stream_id);
        }
    }
}

int stream_add_chunk(const char* stream_id, const char* delta, bool end, const char* error_msg) {
    if (!stream_id) return -1;
    
    stream_session_t* session = stream_get_session(stream_id);
    if (!session) return -1;
    
    // Create new chunk
    stream_chunk_t* chunk = malloc(sizeof(stream_chunk_t));
    if (!chunk) return -1;
    
    memset(chunk, 0, sizeof(stream_chunk_t));
    chunk->seq = session->next_seq++;
    chunk->end = end;
    chunk->error = (error_msg != NULL);
    chunk->next = NULL;
    
    // Copy delta content
    if (delta) {
        chunk->delta = strdup(delta);
        chunk->delta_len = strlen(delta);
        session->total_buffer_size += chunk->delta_len;
    }
    
    // Copy error message
    if (error_msg) {
        chunk->error_message = strdup(error_msg);
    }
    
    // Add to chunk list
    if (!session->chunk_head) {
        session->chunk_head = chunk;
        session->chunk_tail = chunk;
    } else {
        session->chunk_tail->next = chunk;
        session->chunk_tail = chunk;
    }
    
    // Update session state
    if (end || error_msg) {
        session->state = error_msg ? STREAM_STATE_ERROR : STREAM_STATE_FINISHED;
    } else {
        session->state = STREAM_STATE_ACTIVE;
    }
    
    return 0;
}

stream_chunk_t* stream_get_pending_chunks(const char* stream_id, uint32_t from_seq) {
    if (!stream_id) return NULL;
    
    stream_session_t* session = stream_get_session(stream_id);
    if (!session) return NULL;
    
    // Find chunks starting from from_seq
    stream_chunk_t* current = session->chunk_head;
    while (current && current->seq < from_seq) {
        current = current->next;
    }
    
    return current;
}

int stream_mark_chunks_consumed(const char* stream_id, uint32_t up_to_seq) {
    if (!stream_id) return -1;
    
    stream_session_t* session = stream_get_session(stream_id);
    if (!session) return -1;
    
    session->last_consumed_seq = up_to_seq;
    
    // For HTTP polling: Free consumed chunks to save memory
    stream_chunk_t* current = session->chunk_head;
    stream_chunk_t* prev = NULL;
    
    while (current && current->seq <= up_to_seq) {
        stream_chunk_t* next = current->next;
        
        session->total_buffer_size -= current->delta_len;
        free(current->delta);
        free(current->error_message);
        free(current);
        
        current = next;
        if (!prev) {
            session->chunk_head = current;
        }
    }
    
    if (!session->chunk_head) {
        session->chunk_tail = NULL;
    }
    
    return 0;
}

const char* stream_state_to_string(stream_state_t state) {
    switch (state) {
        case STREAM_STATE_INITIALIZING: return "initializing";
        case STREAM_STATE_ACTIVE: return "active";
        case STREAM_STATE_FINISHED: return "finished";
        case STREAM_STATE_ERROR: return "error";
        case STREAM_STATE_EXPIRED: return "expired";
        default: return "unknown";
    }
}

stream_stats_t stream_get_statistics(void) {
    stream_stats_t stats = {0};
    
    if (!g_stream_manager.initialized) return stats;
    
    for (uint32_t i = 0; i < MAX_ACTIVE_STREAMS; i++) {
        stream_session_t* session = &g_stream_manager.sessions[i];
        if (session->initialized) {
            stats.active_sessions++;
            stats.total_memory_used += session->total_buffer_size;
            
            if (stream_is_expired(session)) {
                stats.expired_sessions++;
            }
            
            // Count chunks
            stream_chunk_t* chunk = session->chunk_head;
            while (chunk) {
                stats.total_chunks++;
                chunk = chunk->next;
            }
        }
    }
    
    return stats;
}