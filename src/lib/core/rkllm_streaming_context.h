#ifndef RKLLM_STREAMING_CONTEXT_H
#define RKLLM_STREAMING_CONTEXT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include "../../external/rkllm/rkllm.h"

// Maximum number of concurrent streaming sessions
#define MAX_STREAMING_SESSIONS 16
#define STREAMING_BUFFER_SIZE 16384
#define STREAMING_CHUNK_SIZE 256

// Streaming session states
typedef enum {
    RKLLM_STREAM_STATE_IDLE = 0,
    RKLLM_STREAM_STATE_INITIALIZING,
    RKLLM_STREAM_STATE_STREAMING,
    RKLLM_STREAM_STATE_PAUSED,
    RKLLM_STREAM_STATE_FINISHED,
    RKLLM_STREAM_STATE_ERROR,
    RKLLM_STREAM_STATE_ABORTED
} rkllm_stream_state_t;

// Streaming buffer for efficient token collection
typedef struct {
    char* buffer;               // Ring buffer for streaming tokens
    size_t buffer_size;         // Total buffer capacity
    size_t write_pos;           // Current write position
    size_t read_pos;            // Current read position
    size_t available_bytes;     // Available bytes to read
    pthread_mutex_t mutex;      // Thread safety
    pthread_cond_t data_ready;  // Signal when data is available
    bool overflow;              // Buffer overflow flag
} rkllm_streaming_buffer_t;

// Transport callback function type for real-time streaming
typedef void (*rkllm_transport_callback_t)(const char* session_id, const char* data, 
                                           size_t data_len, bool is_final, void* userdata);

// Enhanced streaming context
typedef struct {
    char session_id[64];                    // Unique session identifier
    uint32_t request_id;                    // JSON-RPC request ID
    rkllm_stream_state_t state;             // Current state
    
    // Threading and synchronization
    pthread_mutex_t state_mutex;           // Protects state changes
    pthread_cond_t state_changed;          // Signal state changes
    pthread_t streaming_thread;            // Dedicated streaming thread
    
    // Buffer management
    rkllm_streaming_buffer_t token_buffer; // Token accumulation buffer
    char* complete_response;               // Full response accumulation
    size_t response_capacity;              // Response buffer capacity
    size_t response_length;                // Current response length
    
    // Callback management
    rkllm_transport_callback_t transport_callback; // Transport-specific callback
    void* transport_userdata;              // Transport callback userdata
    
    // Statistics
    uint64_t start_timestamp;              // Streaming start time
    uint64_t last_token_timestamp;         // Last token received time
    uint32_t total_tokens;                 // Total tokens received
    uint32_t tokens_per_second;            // Current token rate
    
    // Error handling
    int last_error_code;                   // Last RKLLM error code
    char error_message[256];               // Error description
    
    // RKLLM integration
    LLMHandle handle;                      // Associated RKLLM handle
    LLMCallState last_call_state;          // Last RKLLM callback state
    
    bool active;                           // Session is active flag
} rkllm_stream_context_t;

// Global streaming session manager
typedef struct {
    rkllm_stream_context_t sessions[MAX_STREAMING_SESSIONS];
    pthread_mutex_t manager_mutex;         // Protects session management
    uint32_t active_sessions;              // Number of active sessions
    uint32_t next_session_id;              // Session ID counter
    bool initialized;                      // Manager initialization flag
} rkllm_stream_manager_t;

// Function declarations

/**
 * @brief Initialize the streaming context manager
 * @return 0 on success, -1 on error
 */
int rkllm_stream_manager_init(void);

/**
 * @brief Shutdown the streaming context manager
 */
void rkllm_stream_manager_shutdown(void);

/**
 * @brief Create a new streaming session
 * @param request_id JSON-RPC request ID
 * @param handle RKLLM handle
 * @param transport_callback Transport callback function
 * @param userdata Transport callback userdata
 * @return Session context or NULL on error
 */
rkllm_stream_context_t* rkllm_stream_create_session(uint32_t request_id, LLMHandle handle,
                                                    rkllm_transport_callback_t transport_callback,
                                                    void* userdata);

/**
 * @brief Find streaming session by ID
 * @param session_id Session identifier
 * @return Session context or NULL if not found
 */
rkllm_stream_context_t* rkllm_stream_find_session(const char* session_id);

/**
 * @brief Destroy streaming session
 * @param session_id Session identifier
 * @return 0 on success, -1 on error
 */
int rkllm_stream_destroy_session(const char* session_id);

/**
 * @brief Enhanced RKLLM callback for streaming
 * @param result RKLLM result
 * @param userdata Session context pointer
 * @param state RKLLM call state
 * @return 0 to continue, non-zero to abort
 */
int rkllm_enhanced_streaming_callback(RKLLMResult* result, void* userdata, LLMCallState state);

/**
 * @brief Start streaming for a session
 * @param context Session context
 * @return 0 on success, -1 on error
 */
int rkllm_stream_start(rkllm_stream_context_t* context);

/**
 * @brief Pause streaming for a session
 * @param context Session context
 * @return 0 on success, -1 on error
 */
int rkllm_stream_pause(rkllm_stream_context_t* context);

/**
 * @brief Resume streaming for a session
 * @param context Session context
 * @return 0 on success, -1 on error
 */
int rkllm_stream_resume(rkllm_stream_context_t* context);

/**
 * @brief Abort streaming for a session
 * @param context Session context
 * @return 0 on success, -1 on error
 */
int rkllm_stream_abort(rkllm_stream_context_t* context);

/**
 * @brief Get streaming statistics
 * @param context Session context
 * @param stats_json Output JSON string with statistics
 * @return 0 on success, -1 on error
 */
int rkllm_stream_get_stats(rkllm_stream_context_t* context, char** stats_json);

#endif // RKLLM_STREAMING_CONTEXT_H