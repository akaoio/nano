#ifndef NANO_STREAM_MANAGER_H
#define NANO_STREAM_MANAGER_H

#include "common/types.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define STREAM_ID_LENGTH 16
#define MAX_ACTIVE_STREAMS 64
#define STREAM_BUFFER_SIZE 8192
#define STREAM_SESSION_TIMEOUT_SEC 300  // 5 minutes

typedef enum {
    STREAM_STATE_INITIALIZING = 0,
    STREAM_STATE_ACTIVE,
    STREAM_STATE_FINISHED,
    STREAM_STATE_ERROR,
    STREAM_STATE_EXPIRED
} stream_state_t;

typedef struct stream_chunk {
    uint32_t seq;
    char* delta;
    size_t delta_len;
    bool end;
    bool error;
    char* error_message;
    struct stream_chunk* next;
} stream_chunk_t;

typedef struct stream_session {
    char stream_id[STREAM_ID_LENGTH + 1];
    char original_method[64];
    uint32_t request_id;
    stream_state_t state;
    
    // Chunk management
    stream_chunk_t* chunk_head;
    stream_chunk_t* chunk_tail;
    uint32_t next_seq;
    uint32_t last_consumed_seq;
    
    // Session management
    time_t created_time;
    time_t last_access_time;
    bool auto_cleanup;
    
    // Memory management
    size_t total_buffer_size;
    bool initialized;
} stream_session_t;

typedef struct stream_manager {
    stream_session_t sessions[MAX_ACTIVE_STREAMS];
    uint32_t active_count;
    bool initialized;
} stream_manager_t;

// Stream Manager API
int stream_manager_init(void);
void stream_manager_shutdown(void);

// Session Management
stream_session_t* stream_create_session(const char* method, uint32_t request_id);
stream_session_t* stream_get_session(const char* stream_id);
int stream_destroy_session(const char* stream_id);
void stream_cleanup_expired_sessions(void);

// Chunk Management
int stream_add_chunk(const char* stream_id, const char* delta, bool end, const char* error_msg);
stream_chunk_t* stream_get_pending_chunks(const char* stream_id, uint32_t from_seq);
int stream_mark_chunks_consumed(const char* stream_id, uint32_t up_to_seq);

// Utilities
char* stream_generate_id(void);
bool stream_is_expired(const stream_session_t* session);
const char* stream_state_to_string(stream_state_t state);

// Statistics
typedef struct {
    uint32_t active_sessions;
    uint32_t total_chunks;
    size_t total_memory_used;
    uint32_t expired_sessions;
} stream_stats_t;

stream_stats_t stream_get_statistics(void);

#endif