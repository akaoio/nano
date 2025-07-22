#define _POSIX_C_SOURCE 199309L
#include "streaming_buffer_manager.h"
#include "../core/settings_global.h"
#include "../../common/string_utils/string_utils.h"
#include "../../common/time_utils/time_utils.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

// Enable usleep definition
#define _DEFAULT_SOURCE
#include <sys/types.h>

// Helper functions

static int transport_streaming_buffer_init(transport_streaming_buffer_t* buffer,
                                         const transport_stream_config_t* config) {
    if (!buffer || !config) return -1;
    
    memset(buffer, 0, sizeof(transport_streaming_buffer_t));
    buffer->config = *config;
    
    // Initialize circular buffer
    buffer->buffer_size = config->buffer_size;
    buffer->buffer = malloc(buffer->buffer_size);
    if (!buffer->buffer) return -1;
    
    // Initialize chunk queue
    buffer->queue_capacity = config->max_queue_depth;
    buffer->chunk_queue = calloc(buffer->queue_capacity, sizeof(mcp_stream_chunk_t));
    if (!buffer->chunk_queue) {
        free(buffer->buffer);
        return -1;
    }
    
    // Initialize synchronization
    if (pthread_mutex_init(&buffer->buffer_mutex, NULL) != 0) {
        free(buffer->buffer);
        free(buffer->chunk_queue);
        return -1;
    }
    
    if (pthread_cond_init(&buffer->data_available, NULL) != 0) {
        pthread_mutex_destroy(&buffer->buffer_mutex);
        free(buffer->buffer);
        free(buffer->chunk_queue);
        return -1;
    }
    
    if (pthread_cond_init(&buffer->space_available, NULL) != 0) {
        pthread_mutex_destroy(&buffer->buffer_mutex);
        pthread_cond_destroy(&buffer->data_available);
        free(buffer->buffer);
        free(buffer->chunk_queue);
        return -1;
    }
    
    buffer->active = true;
    buffer->last_flush_timestamp = get_timestamp_ms();
    
    printf("✅ Transport streaming buffer initialized: type=%d, chunk_size=%zu, buffer_size=%zu, queue_depth=%zu\n",
           config->type, config->chunk_size, config->buffer_size, config->max_queue_depth);
    
    return 0;
}

static void transport_streaming_buffer_destroy(transport_streaming_buffer_t* buffer) {
    if (!buffer) return;
    
    buffer->active = false;
    
    // Wake up any waiting threads
    pthread_mutex_lock(&buffer->buffer_mutex);
    pthread_cond_broadcast(&buffer->data_available);
    pthread_cond_broadcast(&buffer->space_available);
    pthread_mutex_unlock(&buffer->buffer_mutex);
    
    // Cleanup resources
    pthread_mutex_destroy(&buffer->buffer_mutex);
    pthread_cond_destroy(&buffer->data_available);
    pthread_cond_destroy(&buffer->space_available);
    
    free(buffer->buffer);
    free(buffer->chunk_queue);
    
    printf("🧹 Transport streaming buffer destroyed\n");
}

// Chunk queue management
int transport_chunk_queue_push(transport_streaming_buffer_t* buffer, const mcp_stream_chunk_t* chunk) {
    if (!buffer || !chunk || !buffer->active) return -1;
    
    pthread_mutex_lock(&buffer->buffer_mutex);
    
    if (transport_chunk_queue_is_full(buffer)) {
        buffer->chunks_dropped++;
        pthread_mutex_unlock(&buffer->buffer_mutex);
        return -1; // Queue full
    }
    
    // Copy chunk to queue
    buffer->chunk_queue[buffer->queue_tail] = *chunk;
    buffer->queue_tail = (buffer->queue_tail + 1) % buffer->queue_capacity;
    buffer->queue_size++;
    
    pthread_cond_signal(&buffer->data_available);
    pthread_mutex_unlock(&buffer->buffer_mutex);
    
    return 0;
}

int transport_chunk_queue_pop(transport_streaming_buffer_t* buffer, mcp_stream_chunk_t* chunk) {
    if (!buffer || !chunk || !buffer->active) return -1;
    
    pthread_mutex_lock(&buffer->buffer_mutex);
    
    if (transport_chunk_queue_is_empty(buffer)) {
        pthread_mutex_unlock(&buffer->buffer_mutex);
        return -1; // Queue empty
    }
    
    // Copy chunk from queue
    *chunk = buffer->chunk_queue[buffer->queue_head];
    buffer->queue_head = (buffer->queue_head + 1) % buffer->queue_capacity;
    buffer->queue_size--;
    
    pthread_cond_signal(&buffer->space_available);
    pthread_mutex_unlock(&buffer->buffer_mutex);
    
    return 0;
}

bool transport_chunk_queue_is_full(const transport_streaming_buffer_t* buffer) {
    return buffer->queue_size >= buffer->queue_capacity;
}

bool transport_chunk_queue_is_empty(const transport_streaming_buffer_t* buffer) {
    return buffer->queue_size == 0;
}

// Enhanced transport manager implementation
int enhanced_transport_manager_init(enhanced_transport_manager_t* manager, 
                                   transport_manager_t* base_manager,
                                   const transport_stream_config_t* stream_config) {
    if (!manager || !base_manager || !stream_config) return -1;
    
    memset(manager, 0, sizeof(enhanced_transport_manager_t));
    manager->base_manager = base_manager;
    
    // Initialize streaming buffer
    if (transport_streaming_buffer_init(&manager->streaming_buffer, stream_config) != 0) {
        return -1;
    }
    
    manager->current_sequence = 1;
    manager->start_timestamp = get_timestamp_ms();
    manager->initialized = true;
    
    printf("✅ Enhanced transport manager initialized for %s streaming\n", 
           manager->base_manager->transport->name);
    
    return 0;
}

void enhanced_transport_manager_shutdown(enhanced_transport_manager_t* manager) {
    if (!manager || !manager->initialized) return;
    
    // Stop streaming thread if active
    if (manager->streaming_thread_active) {
        manager->streaming_thread_active = false;
        pthread_join(manager->streaming_thread, NULL);
    }
    
    // Detach stream context
    enhanced_transport_manager_detach_stream(manager);
    
    // Destroy streaming buffer
    transport_streaming_buffer_destroy(&manager->streaming_buffer);
    
    manager->initialized = false;
    printf("🧹 Enhanced transport manager shutdown completed\n");
}

int enhanced_transport_manager_attach_stream(enhanced_transport_manager_t* manager,
                                            rkllm_stream_context_t* stream_context) {
    if (!manager || !stream_context || !manager->initialized) return -1;
    
    // Detach any existing stream
    if (manager->active_stream_context) {
        enhanced_transport_manager_detach_stream(manager);
    }
    
    manager->active_stream_context = stream_context;
    strncpy(manager->current_request_id, stream_context->session_id, sizeof(manager->current_request_id) - 1);
    
    // Set the transport callback in the streaming context
    stream_context->transport_callback = enhanced_transport_streaming_callback;
    stream_context->transport_userdata = manager;
    
    printf("🔗 Attached streaming context %s to transport %s\n", 
           stream_context->session_id, manager->base_manager->transport->name);
    
    return 0;
}

int enhanced_transport_manager_detach_stream(enhanced_transport_manager_t* manager) {
    if (!manager || !manager->active_stream_context) return -1;
    
    // Clear transport callback
    manager->active_stream_context->transport_callback = NULL;
    manager->active_stream_context->transport_userdata = NULL;
    
    printf("🔓 Detached streaming context %s from transport %s\n", 
           manager->current_request_id, manager->base_manager->transport->name);
    
    manager->active_stream_context = NULL;
    manager->current_request_id[0] = '\0';
    
    return 0;
}

// Transport streaming callback - called by RKLLM streaming context
void enhanced_transport_streaming_callback(const char* session_id, const char* data, 
                                          size_t data_len, bool is_final, void* userdata) {
    enhanced_transport_manager_t* manager = (enhanced_transport_manager_t*)userdata;
    
    if (!manager || !session_id) {
        printf("[TRANSPORT_CALLBACK] Invalid parameters\n");
        return;
    }
    
    printf("[TRANSPORT_CALLBACK] Session %s: Received %zu bytes (final=%s) via %s transport\n",
           session_id, data_len, is_final ? "true" : "false", 
           manager->base_manager->transport->name);
    
    // Create MCP stream chunk with original request_id from streaming context
    mcp_stream_chunk_t chunk = {0};
    
    // CRITICAL FIX: Get original request_id from active streaming context
    if (manager->active_stream_context) {
        snprintf(chunk.request_id, sizeof(chunk.request_id), "%u", manager->active_stream_context->request_id);
    }
    
    strncpy(chunk.method, "rkllm_stream", sizeof(chunk.method) - 1);
    chunk.seq = manager->current_sequence++;
    chunk.end = is_final;
    
    // Copy data (truncate if necessary)
    if (data_len > 0 && data) {
        size_t copy_len = (data_len < sizeof(chunk.delta) - 1) ? data_len : sizeof(chunk.delta) - 1;
        memcpy(chunk.delta, data, copy_len);
        chunk.delta[copy_len] = '\0';
    }
    
    // Queue chunk for transport-specific processing
    if (transport_chunk_queue_push(&manager->streaming_buffer, &chunk) != 0) {
        printf("[TRANSPORT_CALLBACK] WARNING: Failed to queue chunk, buffer full\n");
        return;
    }
    
    // Auto-flush if enabled or if final chunk
    if (manager->streaming_buffer.config.auto_flush_enabled || is_final) {
        enhanced_transport_send_chunk_async(manager, data, data_len, is_final);
    }
}

int enhanced_transport_send_chunk_async(enhanced_transport_manager_t* manager,
                                       const char* data, size_t data_len, bool is_final) {
    if (!manager || !manager->initialized) return -1;
    
    // Suppress unused parameter warnings
    (void)data; (void)is_final;
    
    // Get chunk from queue for actual sending
    mcp_stream_chunk_t chunk;
    if (transport_chunk_queue_pop(&manager->streaming_buffer, &chunk) != 0) {
        return -1; // No chunks available
    }
    
    // Send via base transport manager
    int result = transport_manager_send_stream_chunk(manager->base_manager, &chunk);
    
    if (result == TRANSPORT_MANAGER_OK) {
        manager->streaming_buffer.total_chunks_sent++;
        manager->streaming_buffer.total_bytes_sent += data_len;
        
        // Update throughput calculation
        uint64_t elapsed_ms = get_timestamp_ms() - manager->start_timestamp;
        if (elapsed_ms > 0) {
            manager->streaming_buffer.current_throughput = 
                (uint32_t)((manager->streaming_buffer.total_bytes_sent * 1000) / elapsed_ms);
            
            if (manager->streaming_buffer.current_throughput > manager->peak_throughput) {
                manager->peak_throughput = manager->streaming_buffer.current_throughput;
            }
        }
        
        printf("[TRANSPORT_SEND] Sent chunk #%u (%zu bytes) via %s transport\n", 
               chunk.seq, data_len, manager->base_manager->transport->name);
    } else {
        printf("[TRANSPORT_SEND] Failed to send chunk via %s transport (result: %d)\n", 
               manager->base_manager->transport->name, result);
    }
    
    return (result == TRANSPORT_MANAGER_OK) ? 0 : -1;
}

int enhanced_transport_flush_chunks(enhanced_transport_manager_t* manager) {
    if (!manager || !manager->initialized) return -1;
    
    int chunks_sent = 0;
    mcp_stream_chunk_t chunk;
    
    // Send all queued chunks
    while (transport_chunk_queue_pop(&manager->streaming_buffer, &chunk) == 0) {
        if (transport_manager_send_stream_chunk(manager->base_manager, &chunk) == TRANSPORT_MANAGER_OK) {
            chunks_sent++;
            manager->streaming_buffer.total_chunks_sent++;
        }
    }
    
    manager->streaming_buffer.last_flush_timestamp = get_timestamp_ms();
    
    if (chunks_sent > 0) {
        printf("[TRANSPORT_FLUSH] Flushed %d pending chunks via %s transport\n", 
               chunks_sent, manager->base_manager->transport->name);
    }
    
    return 0;
}

// Get optimal streaming configuration for different transport types
int enhanced_transport_get_optimal_config(const char* transport_name, 
                                         transport_stream_config_t* config) {
    if (!transport_name || !config) return -1;
    
    // Default configuration
    memset(config, 0, sizeof(transport_stream_config_t));
    config->buffer_size = 16384;
    config->max_queue_depth = 64;
    config->flush_interval_ms = 100;
    config->auto_flush_enabled = true;
    
    // Transport-specific optimizations
    if (strcmp(transport_name, "websocket") == 0) {
        config->type = TRANSPORT_STREAM_TYPE_WEBSOCKET;
        config->chunk_size = 1024;              // WebSocket frame size
        config->flush_interval_ms = 50;         // Real-time streaming
        config->keep_alive_enabled = true;
        config->keep_alive_interval_ms = 30000; // 30 second ping
        
    } else if (strcmp(transport_name, "http") == 0) {
        config->type = TRANSPORT_STREAM_TYPE_HTTP_SSE;
        config->chunk_size = 2048;              // SSE event size
        config->flush_interval_ms = 200;        // Balanced for HTTP
        config->compression_enabled = true;     // Enable gzip
        
    } else if (strcmp(transport_name, "tcp") == 0) {
        config->type = TRANSPORT_STREAM_TYPE_TCP_FRAMED;
        config->chunk_size = 4096;              // TCP optimal size
        config->flush_interval_ms = 100;
        config->buffer_size = 32768;            // Larger buffer for TCP
        
    } else if (strcmp(transport_name, "udp") == 0) {
        config->type = TRANSPORT_STREAM_TYPE_UDP_PACKET;
        config->chunk_size = 1400;              // Under MTU size
        config->flush_interval_ms = 10;         // Fast flush for UDP
        config->max_queue_depth = 128;          // Handle packet loss
        
    } else if (strcmp(transport_name, "stdio") == 0) {
        config->type = TRANSPORT_STREAM_TYPE_STDIO_LINE;
        config->chunk_size = 512;               // Line-based chunks
        config->flush_interval_ms = 0;          // Immediate flush
        config->auto_flush_enabled = true;
        
    } else {
        return -1; // Unknown transport
    }
    
    printf("📋 Optimal streaming config for %s: chunk_size=%zu, buffer_size=%zu, flush_interval=%u ms\n",
           transport_name, config->chunk_size, config->buffer_size, config->flush_interval_ms);
    
    return 0;
}

int enhanced_transport_get_streaming_stats(enhanced_transport_manager_t* manager, char** stats_json) {
    if (!manager || !stats_json) return -1;
    
    json_object* stats = json_object_new_object();
    json_object_object_add(stats, "transport_name", 
                          json_object_new_string(manager->base_manager->transport->name));
    json_object_object_add(stats, "current_request_id", 
                          json_object_new_string(manager->current_request_id));
    json_object_object_add(stats, "total_chunks_sent", 
                          json_object_new_int64(manager->streaming_buffer.total_chunks_sent));
    json_object_object_add(stats, "total_bytes_sent", 
                          json_object_new_int64(manager->streaming_buffer.total_bytes_sent));
    json_object_object_add(stats, "chunks_dropped", 
                          json_object_new_int64(manager->streaming_buffer.chunks_dropped));
    json_object_object_add(stats, "current_throughput_bps", 
                          json_object_new_int(manager->streaming_buffer.current_throughput));
    json_object_object_add(stats, "peak_throughput_bps", 
                          json_object_new_int(manager->peak_throughput));
    json_object_object_add(stats, "queue_size", 
                          json_object_new_int(manager->streaming_buffer.queue_size));
    json_object_object_add(stats, "queue_capacity", 
                          json_object_new_int(manager->streaming_buffer.queue_capacity));
    
    uint64_t elapsed_ms = get_timestamp_ms() - manager->start_timestamp;
    json_object_object_add(stats, "uptime_ms", json_object_new_int64(elapsed_ms));
    
    // Transport-specific stats
    json_object* transport_config = json_object_new_object();
    json_object_object_add(transport_config, "type", 
                          json_object_new_int(manager->streaming_buffer.config.type));
    json_object_object_add(transport_config, "chunk_size", 
                          json_object_new_int(manager->streaming_buffer.config.chunk_size));
    json_object_object_add(transport_config, "auto_flush_enabled", 
                          json_object_new_boolean(manager->streaming_buffer.config.auto_flush_enabled));
    json_object_object_add(stats, "transport_config", transport_config);
    
    const char* json_str = json_object_to_json_string(stats);
    *stats_json = strdup(json_str);
    json_object_put(stats);
    
    return 0;
}

int enhanced_transport_handle_stream_event(enhanced_transport_manager_t* manager,
                                          const char* event_type) {
    if (!manager || !event_type) return -1;
    
    printf("[TRANSPORT_EVENT] %s transport event: %s\n", 
           manager->base_manager->transport->name, event_type);
    
    if (strcmp(event_type, "connect") == 0) {
        // Reset statistics on new connection
        manager->streaming_buffer.total_chunks_sent = 0;
        manager->streaming_buffer.total_bytes_sent = 0;
        manager->streaming_buffer.chunks_dropped = 0;
        manager->start_timestamp = get_timestamp_ms();
        
    } else if (strcmp(event_type, "disconnect") == 0) {
        // Flush any pending chunks before disconnect
        enhanced_transport_flush_chunks(manager);
        
    } else if (strcmp(event_type, "error") == 0) {
        // Handle transport errors
        if (manager->active_stream_context) {
            // Notify streaming context of transport error
            rkllm_stream_abort(manager->active_stream_context);
        }
    }
    
    return 0;
}

// Generic streaming worker with transport-specific formatting
static int format_chunk_for_transport(transport_stream_type_t type, const mcp_stream_chunk_t* chunk, 
                                     char* output, size_t output_size) {
    switch (type) {
        case TRANSPORT_STREAM_TYPE_WEBSOCKET:
            return snprintf(output, output_size,
                "{\"jsonrpc\":\"2.0\",\"method\":\"%s\","
                "\"params\":{\"seq\":%u,\"delta\":\"%s\",\"end\":%s}}",
                chunk->method, chunk->seq, chunk->delta, chunk->end ? "true" : "false");
                
        case TRANSPORT_STREAM_TYPE_HTTP_SSE:
            return snprintf(output, output_size,
                "id: %u\nevent: stream_chunk\n"
                "data: {\"seq\":%u,\"delta\":\"%s\",\"end\":%s}\n\n",
                chunk->seq, chunk->seq, chunk->delta, chunk->end ? "true" : "false");
                
        case TRANSPORT_STREAM_TYPE_TCP_FRAMED: {
            char json_part[3800];
            int json_len = snprintf(json_part, sizeof(json_part),
                "{\"seq\":%u,\"delta\":\"%s\",\"end\":%s}",
                chunk->seq, chunk->delta, chunk->end ? "true" : "false");
            
            if (json_len + sizeof(uint32_t) >= output_size) return -1;
            
            // Frame format: 4-byte length + JSON data
            uint32_t length = (uint32_t)json_len;
            memcpy(output, &length, sizeof(uint32_t));
            memcpy(output + sizeof(uint32_t), json_part, json_len);
            return sizeof(uint32_t) + json_len;
        }
        
        default:
            return snprintf(output, output_size, "{\"error\":\"unsupported_transport\"}");
    }
}

void* generic_streaming_worker(void* arg) {
    enhanced_transport_manager_t* manager = (enhanced_transport_manager_t*)arg;
    if (!manager) return NULL;
    
    const char* transport_name = manager->base_manager->transport->name;
    printf("[STREAM_WORKER] Started worker for %s transport\n", transport_name);
    
    mcp_stream_chunk_t chunk;
    char formatted_data[4096];
    
    while (manager->streaming_thread_active && manager->streaming_buffer.active) {
        // Try to get chunk from queue
        if (transport_chunk_queue_pop(&manager->streaming_buffer, &chunk) == 0) {
            // Format chunk based on transport type
            int data_len = format_chunk_for_transport(manager->streaming_buffer.config.type, 
                                                    &chunk, formatted_data, sizeof(formatted_data));
            
            if (data_len > 0) {
                // Send via transport
                if (manager->base_manager->transport->send(manager->base_manager->transport, 
                                                         formatted_data, data_len) != 0) {
                    printf("[STREAM_WORKER] Failed to send via %s transport\n", transport_name);
                }
            } else {
                printf("[STREAM_WORKER] Failed to format chunk for %s\n", transport_name);
            }
        }
        
        // Handle keep-alive for WebSocket
        if (manager->streaming_buffer.config.keep_alive_enabled && 
            manager->streaming_buffer.config.type == TRANSPORT_STREAM_TYPE_WEBSOCKET) {
            uint64_t now = get_timestamp_ms();
            if (now - manager->streaming_buffer.last_flush_timestamp > 
                manager->streaming_buffer.config.keep_alive_interval_ms) {
                
                const char* ping = "{\"type\":\"ping\"}";
                manager->base_manager->transport->send(manager->base_manager->transport, ping, strlen(ping));
                manager->streaming_buffer.last_flush_timestamp = now;
            }
        }
        
        // Sleep for configured interval
        struct timespec sleep_time = {
            .tv_sec = manager->streaming_buffer.config.flush_interval_ms / 1000,
            .tv_nsec = (manager->streaming_buffer.config.flush_interval_ms % 1000) * 1000000
        };
        nanosleep(&sleep_time, NULL);
    }
    
    printf("[STREAM_WORKER] Worker stopped for %s transport\n", transport_name);
    return NULL;
}

// Legacy function aliases for compatibility
void* websocket_streaming_worker(void* arg) { return generic_streaming_worker(arg); }
void* http_sse_streaming_worker(void* arg) { return generic_streaming_worker(arg); }
void* tcp_streaming_worker(void* arg) { return generic_streaming_worker(arg); }