#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include "../../external/rkllm/rkllm.h"
#include "../../include/mcp/transport.h"

// Using void* to avoid circular dependency with adapter.h

// Stream session structure - using stream_manager specific name to avoid conflict
typedef struct stream_manager_session {
    char session_id[64];
    char request_id[32];
    transport_type_t transport_type;
    void* transport_handle;
    void* rkllm_context;  // Changed from rkllm_stream_context_t* to void* to avoid circular dependencies
    pthread_mutex_t session_mutex;
    bool active;
    uint32_t sequence_number;
    uint64_t created_timestamp;
    uint64_t last_activity;
} stream_manager_session_t;

// Stream manager structure
typedef struct {
    stream_manager_session_t* sessions;
    size_t session_count;
    size_t max_sessions;
    pthread_mutex_t manager_mutex;
    pthread_t cleanup_thread;
    bool initialized;
    bool running;
} stream_manager_t;

// Use existing mcp_stream_chunk_t from adapter.h

/**
 * @brief Initialize the stream manager
 * @param manager Stream manager instance to initialize
 * @return 0 on success, -1 on error
 */
int stream_manager_init(stream_manager_t* manager);

/**
 * @brief Shutdown the stream manager
 * @param manager Stream manager instance to shutdown
 */
void stream_manager_shutdown(stream_manager_t* manager);

/**
 * @brief Create a new streaming session
 * @param request_id JSON-RPC request ID
 * @param type Transport type
 * @return Pointer to created session or NULL on error
 */
stream_manager_session_t* stream_manager_create_session(const char* request_id, transport_type_t type);

/**
 * @brief Get session by session ID
 * @param session_id Session ID to look up
 * @return Pointer to session or NULL if not found
 */
stream_manager_session_t* stream_manager_get_session(const char* session_id);

/**
 * @brief Destroy a streaming session
 * @param session_id Session ID to destroy
 * @return 0 on success, -1 on error
 */
int stream_manager_destroy_session(const char* session_id);

/**
 * @brief Clean up expired sessions (called by cleanup thread)
 * @return Number of sessions cleaned up
 */
int stream_manager_cleanup_expired_sessions(void);

/**
 * @brief Generate a unique session ID
 * @param session_id Output buffer for session ID
 * @param buffer_size Size of output buffer
 */
void generate_session_id(char* session_id, size_t buffer_size);

/**
 * @brief Get current timestamp in milliseconds
 * @return Current timestamp in milliseconds
 */
uint64_t get_timestamp_ms(void);

/**
 * @brief Get the global stream manager instance
 * @return Pointer to global stream manager
 */
stream_manager_t* get_global_stream_manager(void);

/**
 * @brief Forward stream chunk to transport layer
 * @param transport_handle Transport-specific handle
 * @param chunk Stream chunk to send
 * @return 0 on success, -1 on error
 */
int transport_send_stream_chunk(void* transport_handle, void* chunk);