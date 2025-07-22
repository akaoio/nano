#ifndef STREAMING_BUFFER_MANAGER_H
#define STREAMING_BUFFER_MANAGER_H

#include "manager.h"
#include "../core/rkllm_streaming_context.h"
#include "../protocol/adapter.h"
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// Transport-specific streaming configurations
typedef enum {
    TRANSPORT_STREAM_TYPE_WEBSOCKET = 0,    // Real-time streaming with frames
    TRANSPORT_STREAM_TYPE_HTTP_SSE,         // Server-Sent Events
    TRANSPORT_STREAM_TYPE_HTTP_CHUNKED,     // HTTP chunked transfer
    TRANSPORT_STREAM_TYPE_TCP_FRAMED,       // TCP with custom framing
    TRANSPORT_STREAM_TYPE_UDP_PACKET,       // UDP packet-based
    TRANSPORT_STREAM_TYPE_STDIO_LINE,       // STDIO line-based
} transport_stream_type_t;

// Transport-specific streaming buffer configuration
typedef struct {
    transport_stream_type_t type;
    size_t chunk_size;              // Optimal chunk size for this transport
    size_t buffer_size;             // Total buffer size
    size_t max_queue_depth;         // Maximum queued chunks
    uint32_t flush_interval_ms;     // Automatic flush interval
    bool auto_flush_enabled;        // Auto-flush on chunk boundaries
    bool compression_enabled;       // Enable compression (if supported)
    bool keep_alive_enabled;        // Send keep-alive packets
    uint32_t keep_alive_interval_ms; // Keep-alive interval
} transport_stream_config_t;

// Transport streaming buffer for efficient chunk management
typedef struct {
    char* buffer;                   // Circular buffer for outgoing chunks
    size_t buffer_size;
    size_t write_pos;
    size_t read_pos;
    size_t available_data;
    
    // Chunk queue for transport-specific formatting
    mcp_stream_chunk_t* chunk_queue;
    size_t queue_capacity;
    size_t queue_size;
    size_t queue_head;
    size_t queue_tail;
    
    // Threading and synchronization
    pthread_mutex_t buffer_mutex;
    pthread_cond_t data_available;
    pthread_cond_t space_available;
    
    // Transport-specific configuration
    transport_stream_config_t config;
    
    // Statistics
    uint64_t total_chunks_sent;
    uint64_t total_bytes_sent;
    uint64_t chunks_dropped;        // Due to buffer full
    uint64_t last_flush_timestamp;
    uint32_t current_throughput;    // Bytes per second
    
    bool active;
    bool flush_pending;
} transport_streaming_buffer_t;

// Enhanced transport manager with streaming support
typedef struct {
    transport_manager_t* base_manager;
    transport_streaming_buffer_t streaming_buffer;
    
    // Streaming context integration
    rkllm_stream_context_t* active_stream_context;
    
    // Worker thread for async streaming
    pthread_t streaming_thread;
    bool streaming_thread_active;
    
    // Transport-specific streaming state
    uint32_t current_sequence;
    char current_request_id[64];
    
    // Performance monitoring
    uint64_t start_timestamp;
    uint32_t peak_throughput;
    
    bool initialized;
} enhanced_transport_manager_t;

// Function declarations

/**
 * @brief Initialize enhanced transport manager with streaming support
 * @param manager Enhanced transport manager
 * @param base_manager Base transport manager
 * @param stream_config Streaming configuration
 * @return 0 on success, -1 on error
 */
int enhanced_transport_manager_init(enhanced_transport_manager_t* manager, 
                                   transport_manager_t* base_manager,
                                   const transport_stream_config_t* stream_config);

/**
 * @brief Shutdown enhanced transport manager
 * @param manager Enhanced transport manager
 */
void enhanced_transport_manager_shutdown(enhanced_transport_manager_t* manager);

/**
 * @brief Attach streaming context to transport manager
 * @param manager Enhanced transport manager
 * @param stream_context RKLLM streaming context
 * @return 0 on success, -1 on error
 */
int enhanced_transport_manager_attach_stream(enhanced_transport_manager_t* manager,
                                            rkllm_stream_context_t* stream_context);

/**
 * @brief Detach streaming context from transport manager
 * @param manager Enhanced transport manager
 * @return 0 on success, -1 on error
 */
int enhanced_transport_manager_detach_stream(enhanced_transport_manager_t* manager);

/**
 * @brief Transport callback for streaming context
 * This is called by the RKLLM streaming context when tokens are available
 * @param request_id JSON-RPC request ID (reused for streaming)
 * @param data Token data
 * @param data_len Token data length
 * @param is_final Whether this is the final token
 * @param userdata Enhanced transport manager pointer
 */
void enhanced_transport_streaming_callback(const char* request_id, const char* data, 
                                          size_t data_len, bool is_final, void* userdata);

/**
 * @brief Send streaming chunk via transport (async)
 * @param manager Enhanced transport manager
 * @param data Chunk data
 * @param data_len Data length
 * @param is_final Whether this is the final chunk
 * @return 0 on success, -1 on error
 */
int enhanced_transport_send_chunk_async(enhanced_transport_manager_t* manager,
                                       const char* data, size_t data_len, bool is_final);

/**
 * @brief Force flush pending streaming chunks
 * @param manager Enhanced transport manager
 * @return 0 on success, -1 on error
 */
int enhanced_transport_flush_chunks(enhanced_transport_manager_t* manager);

/**
 * @brief Get transport-specific streaming configuration for transport type
 * @param transport_name Transport name (e.g., "websocket", "http", "tcp")
 * @param config Output configuration
 * @return 0 on success, -1 on error
 */
int enhanced_transport_get_optimal_config(const char* transport_name, 
                                         transport_stream_config_t* config);

/**
 * @brief Get streaming statistics
 * @param manager Enhanced transport manager
 * @param stats_json Output JSON string with statistics
 * @return 0 on success, -1 on error
 */
int enhanced_transport_get_streaming_stats(enhanced_transport_manager_t* manager,
                                          char** stats_json);

/**
 * @brief Handle streaming connection events (connect/disconnect)
 * @param manager Enhanced transport manager
 * @param event_type Event type ("connect", "disconnect", "error")
 * @return 0 on success, -1 on error
 */
int enhanced_transport_handle_stream_event(enhanced_transport_manager_t* manager,
                                          const char* event_type);

// Transport-specific streaming workers
void* websocket_streaming_worker(void* arg);
void* http_sse_streaming_worker(void* arg);
void* tcp_streaming_worker(void* arg);

// Utility functions for chunk management
int transport_chunk_queue_push(transport_streaming_buffer_t* buffer, 
                              const mcp_stream_chunk_t* chunk);
int transport_chunk_queue_pop(transport_streaming_buffer_t* buffer, 
                             mcp_stream_chunk_t* chunk);
bool transport_chunk_queue_is_full(const transport_streaming_buffer_t* buffer);
bool transport_chunk_queue_is_empty(const transport_streaming_buffer_t* buffer);

#endif // STREAMING_BUFFER_MANAGER_H