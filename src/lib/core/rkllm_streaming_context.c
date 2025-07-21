#define _POSIX_C_SOURCE 199309L
#include "rkllm_streaming_context.h"
#include "rkllm_error_mapping.h"
#include "../../common/string_utils/string_utils.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

// Global streaming manager instance
static rkllm_stream_manager_t g_stream_manager = {0};

// Helper functions
static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void generate_session_id(char* session_id, size_t size) {
    snprintf(session_id, size, "rkllm_stream_%u_%lu", 
             g_stream_manager.next_session_id++, get_timestamp_ms());
}

// Streaming buffer functions
static int streaming_buffer_init(rkllm_streaming_buffer_t* buffer, size_t size) {
    buffer->buffer = malloc(size);
    if (!buffer->buffer) {
        return -1;
    }
    
    buffer->buffer_size = size;
    buffer->write_pos = 0;
    buffer->read_pos = 0;
    buffer->available_bytes = 0;
    buffer->overflow = false;
    
    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        free(buffer->buffer);
        return -1;
    }
    
    if (pthread_cond_init(&buffer->data_ready, NULL) != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->buffer);
        return -1;
    }
    
    return 0;
}

static void streaming_buffer_destroy(rkllm_streaming_buffer_t* buffer) {
    if (buffer->buffer) {
        free(buffer->buffer);
        buffer->buffer = NULL;
    }
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->data_ready);
}

static int streaming_buffer_write(rkllm_streaming_buffer_t* buffer, const char* data, size_t len) {
    pthread_mutex_lock(&buffer->mutex);
    
    // Check if we have space
    size_t free_space = buffer->buffer_size - buffer->available_bytes;
    if (len > free_space) {
        buffer->overflow = true;
        pthread_mutex_unlock(&buffer->mutex);
        return -1; // Buffer full
    }
    
    // Write data (handle wrap-around)
    size_t first_chunk = buffer->buffer_size - buffer->write_pos;
    if (len <= first_chunk) {
        // Data fits before wrap-around
        memcpy(buffer->buffer + buffer->write_pos, data, len);
        buffer->write_pos = (buffer->write_pos + len) % buffer->buffer_size;
    } else {
        // Data requires wrap-around
        memcpy(buffer->buffer + buffer->write_pos, data, first_chunk);
        memcpy(buffer->buffer, data + first_chunk, len - first_chunk);
        buffer->write_pos = len - first_chunk;
    }
    
    buffer->available_bytes += len;
    pthread_cond_signal(&buffer->data_ready);
    pthread_mutex_unlock(&buffer->mutex);
    
    return 0;
}

static int streaming_buffer_read(rkllm_streaming_buffer_t* buffer, char* data, size_t max_len) {
    pthread_mutex_lock(&buffer->mutex);
    
    if (buffer->available_bytes == 0) {
        pthread_mutex_unlock(&buffer->mutex);
        return 0; // No data available
    }
    
    size_t read_len = (buffer->available_bytes < max_len) ? buffer->available_bytes : max_len;
    
    // Read data (handle wrap-around)
    size_t first_chunk = buffer->buffer_size - buffer->read_pos;
    if (read_len <= first_chunk) {
        // Data fits before wrap-around
        memcpy(data, buffer->buffer + buffer->read_pos, read_len);
        buffer->read_pos = (buffer->read_pos + read_len) % buffer->buffer_size;
    } else {
        // Data requires wrap-around
        memcpy(data, buffer->buffer + buffer->read_pos, first_chunk);
        memcpy(data + first_chunk, buffer->buffer, read_len - first_chunk);
        buffer->read_pos = read_len - first_chunk;
    }
    
    buffer->available_bytes -= read_len;
    pthread_mutex_unlock(&buffer->mutex);
    
    return (int)read_len;
}

// Session management functions
int rkllm_stream_manager_init(void) {
    if (g_stream_manager.initialized) {
        return 0;
    }
    
    memset(&g_stream_manager, 0, sizeof(g_stream_manager));
    
    if (pthread_mutex_init(&g_stream_manager.manager_mutex, NULL) != 0) {
        return -1;
    }
    
    g_stream_manager.active_sessions = 0;
    g_stream_manager.next_session_id = 1;
    g_stream_manager.initialized = true;
    
    printf("✅ RKLLM streaming context manager initialized (max sessions: %d)\n", MAX_STREAMING_SESSIONS);
    return 0;
}

void rkllm_stream_manager_shutdown(void) {
    if (!g_stream_manager.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_stream_manager.manager_mutex);
    
    // Destroy all active sessions
    for (int i = 0; i < MAX_STREAMING_SESSIONS; i++) {
        if (g_stream_manager.sessions[i].active) {
            rkllm_stream_abort(&g_stream_manager.sessions[i]);
            streaming_buffer_destroy(&g_stream_manager.sessions[i].token_buffer);
            if (g_stream_manager.sessions[i].complete_response) {
                free(g_stream_manager.sessions[i].complete_response);
            }
            pthread_mutex_destroy(&g_stream_manager.sessions[i].state_mutex);
            pthread_cond_destroy(&g_stream_manager.sessions[i].state_changed);
        }
    }
    
    pthread_mutex_unlock(&g_stream_manager.manager_mutex);
    pthread_mutex_destroy(&g_stream_manager.manager_mutex);
    
    g_stream_manager.initialized = false;
    printf("🧹 RKLLM streaming context manager shutdown completed\n");
}

rkllm_stream_context_t* rkllm_stream_create_session(uint32_t request_id, LLMHandle handle,
                                                    rkllm_transport_callback_t transport_callback,
                                                    void* userdata) {
    if (!g_stream_manager.initialized) {
        rkllm_stream_manager_init();
    }
    
    pthread_mutex_lock(&g_stream_manager.manager_mutex);
    
    // Find free session slot
    rkllm_stream_context_t* context = NULL;
    for (int i = 0; i < MAX_STREAMING_SESSIONS; i++) {
        if (!g_stream_manager.sessions[i].active) {
            context = &g_stream_manager.sessions[i];
            break;
        }
    }
    
    if (!context) {
        pthread_mutex_unlock(&g_stream_manager.manager_mutex);
        return NULL; // No free slots
    }
    
    // Initialize session
    memset(context, 0, sizeof(rkllm_stream_context_t));
    generate_session_id(context->session_id, sizeof(context->session_id));
    context->request_id = request_id;
    context->state = RKLLM_STREAM_STATE_IDLE;
    context->handle = handle;
    context->transport_callback = transport_callback;
    context->transport_userdata = userdata;
    context->start_timestamp = get_timestamp_ms();
    context->active = true;
    
    // Initialize mutexes and conditions
    if (pthread_mutex_init(&context->state_mutex, NULL) != 0) {
        pthread_mutex_unlock(&g_stream_manager.manager_mutex);
        return NULL;
    }
    
    if (pthread_cond_init(&context->state_changed, NULL) != 0) {
        pthread_mutex_destroy(&context->state_mutex);
        pthread_mutex_unlock(&g_stream_manager.manager_mutex);
        return NULL;
    }
    
    // Initialize streaming buffer
    if (streaming_buffer_init(&context->token_buffer, STREAMING_BUFFER_SIZE) != 0) {
        pthread_mutex_destroy(&context->state_mutex);
        pthread_cond_destroy(&context->state_changed);
        pthread_mutex_unlock(&g_stream_manager.manager_mutex);
        return NULL;
    }
    
    // Initialize response accumulation buffer
    context->response_capacity = STREAMING_BUFFER_SIZE * 2;
    context->complete_response = malloc(context->response_capacity);
    if (!context->complete_response) {
        streaming_buffer_destroy(&context->token_buffer);
        pthread_mutex_destroy(&context->state_mutex);
        pthread_cond_destroy(&context->state_changed);
        pthread_mutex_unlock(&g_stream_manager.manager_mutex);
        return NULL;
    }
    context->complete_response[0] = '\0';
    context->response_length = 0;
    
    g_stream_manager.active_sessions++;
    pthread_mutex_unlock(&g_stream_manager.manager_mutex);
    
    printf("🆕 Created streaming session: %s (request_id: %u, active: %u/%d)\n", 
           context->session_id, request_id, g_stream_manager.active_sessions, MAX_STREAMING_SESSIONS);
    
    return context;
}

rkllm_stream_context_t* rkllm_stream_find_session(const char* session_id) {
    if (!g_stream_manager.initialized || !session_id) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_stream_manager.manager_mutex);
    
    for (int i = 0; i < MAX_STREAMING_SESSIONS; i++) {
        if (g_stream_manager.sessions[i].active && 
            strcmp(g_stream_manager.sessions[i].session_id, session_id) == 0) {
            pthread_mutex_unlock(&g_stream_manager.manager_mutex);
            return &g_stream_manager.sessions[i];
        }
    }
    
    pthread_mutex_unlock(&g_stream_manager.manager_mutex);
    return NULL;
}

int rkllm_stream_destroy_session(const char* session_id) {
    rkllm_stream_context_t* context = rkllm_stream_find_session(session_id);
    if (!context) {
        return -1;
    }
    
    pthread_mutex_lock(&g_stream_manager.manager_mutex);
    pthread_mutex_lock(&context->state_mutex);
    
    // Abort streaming if active
    if (context->state == RKLLM_STREAM_STATE_STREAMING) {
        context->state = RKLLM_STREAM_STATE_ABORTED;
        pthread_cond_broadcast(&context->state_changed);
    }
    
    // Clean up resources
    streaming_buffer_destroy(&context->token_buffer);
    if (context->complete_response) {
        free(context->complete_response);
        context->complete_response = NULL;
    }
    
    context->active = false;
    g_stream_manager.active_sessions--;
    
    pthread_mutex_unlock(&context->state_mutex);
    pthread_mutex_destroy(&context->state_mutex);
    pthread_cond_destroy(&context->state_changed);
    
    pthread_mutex_unlock(&g_stream_manager.manager_mutex);
    
    printf("🗑️  Destroyed streaming session: %s (remaining: %u)\n", 
           session_id, g_stream_manager.active_sessions);
    
    return 0;
}

// Enhanced streaming callback - the heart of the streaming system
int rkllm_enhanced_streaming_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    rkllm_stream_context_t* context = (rkllm_stream_context_t*)userdata;
    
    if (!context || !context->active) {
        printf("[STREAM_CALLBACK] Invalid or inactive session context\n");
        return -1; // Abort inference
    }
    
    pthread_mutex_lock(&context->state_mutex);
    
    context->last_call_state = state;
    context->last_token_timestamp = get_timestamp_ms();
    
    printf("[STREAM_CALLBACK] Session %s: State %d, Result ptr: %p\n", 
           context->session_id, state, result);
    
    if (!result) {
        printf("[STREAM_CALLBACK] Session %s: NULL result pointer\n", context->session_id);
        context->state = RKLLM_STREAM_STATE_ERROR;
        context->last_error_code = -1;
        snprintf(context->error_message, sizeof(context->error_message), "NULL result in callback");
        pthread_cond_broadcast(&context->state_changed);
        pthread_mutex_unlock(&context->state_mutex);
        return -1;
    }
    
    // Handle token data
    if (result->text && strlen(result->text) > 0) {
        size_t text_len = strlen(result->text);
        context->total_tokens++;
        
        printf("[STREAM_CALLBACK] Session %s: Token #%u: '%.100s'%s\n", 
               context->session_id, context->total_tokens, result->text, 
               text_len > 100 ? "..." : "");
        
        // Add to streaming buffer
        if (streaming_buffer_write(&context->token_buffer, result->text, text_len) == 0) {
            // Add to complete response
            if (context->response_length + text_len < context->response_capacity - 1) {
                strcat(context->complete_response, result->text);
                context->response_length += text_len;
            }
            
            // Call transport callback for real-time streaming
            if (context->transport_callback) {
                context->transport_callback(context->session_id, result->text, text_len, 
                                          false, context->transport_userdata);
            }
        } else {
            printf("[STREAM_CALLBACK] Session %s: Buffer overflow, token dropped\n", context->session_id);
        }
    }
    
    // Handle token ID
    if (result->token_id != 0) {
        printf("[STREAM_CALLBACK] Session %s: Token ID: %d\n", context->session_id, result->token_id);
    }
    
    // Handle state transitions
    switch (state) {
        case RKLLM_RUN_NORMAL:
            if (context->state == RKLLM_STREAM_STATE_IDLE) {
                context->state = RKLLM_STREAM_STATE_STREAMING;
                printf("[STREAM_CALLBACK] Session %s: Started streaming\n", context->session_id);
            }
            break;
            
        case RKLLM_RUN_WAITING:
            // Model is waiting for input or in processing state
            if (context->state == RKLLM_STREAM_STATE_IDLE) {
                context->state = RKLLM_STREAM_STATE_INITIALIZING;
                printf("[STREAM_CALLBACK] Session %s: Initializing\n", context->session_id);
            }
            break;
            
        case RKLLM_RUN_FINISH:
            context->state = RKLLM_STREAM_STATE_FINISHED;
            printf("[STREAM_CALLBACK] Session %s: Finished (total tokens: %u)\n", 
                   context->session_id, context->total_tokens);
            
            // Final transport callback
            if (context->transport_callback) {
                context->transport_callback(context->session_id, "", 0, 
                                          true, context->transport_userdata);
            }
            break;
            
        case RKLLM_RUN_ERROR:
            context->state = RKLLM_STREAM_STATE_ERROR;
            context->last_error_code = -8; // RKLLM_ERROR_INFERENCE_FAILED
            snprintf(context->error_message, sizeof(context->error_message), "RKLLM inference error");
            printf("[STREAM_CALLBACK] Session %s: Error occurred\n", context->session_id);
            break;
    }
    
    // Calculate tokens per second
    uint64_t elapsed_ms = context->last_token_timestamp - context->start_timestamp;
    if (elapsed_ms > 0) {
        context->tokens_per_second = (uint32_t)((context->total_tokens * 1000) / elapsed_ms);
    }
    
    pthread_cond_broadcast(&context->state_changed);
    pthread_mutex_unlock(&context->state_mutex);
    
    // Check for abort signal
    if (context->state == RKLLM_STREAM_STATE_ABORTED) {
        printf("[STREAM_CALLBACK] Session %s: Aborted by user\n", context->session_id);
        return -1; // Abort inference
    }
    
    return 0; // Continue inference
}

// Session control functions
int rkllm_stream_start(rkllm_stream_context_t* context) {
    if (!context) return -1;
    
    pthread_mutex_lock(&context->state_mutex);
    
    if (context->state != RKLLM_STREAM_STATE_IDLE) {
        pthread_mutex_unlock(&context->state_mutex);
        return -1; // Invalid state
    }
    
    context->state = RKLLM_STREAM_STATE_INITIALIZING;
    context->start_timestamp = get_timestamp_ms();
    
    printf("🎬 Started streaming session: %s\n", context->session_id);
    pthread_cond_broadcast(&context->state_changed);
    pthread_mutex_unlock(&context->state_mutex);
    
    return 0;
}

int rkllm_stream_pause(rkllm_stream_context_t* context) {
    if (!context) return -1;
    
    pthread_mutex_lock(&context->state_mutex);
    
    if (context->state == RKLLM_STREAM_STATE_STREAMING) {
        context->state = RKLLM_STREAM_STATE_PAUSED;
        printf("⏸️  Paused streaming session: %s\n", context->session_id);
        pthread_cond_broadcast(&context->state_changed);
    }
    
    pthread_mutex_unlock(&context->state_mutex);
    return 0;
}

int rkllm_stream_resume(rkllm_stream_context_t* context) {
    if (!context) return -1;
    
    pthread_mutex_lock(&context->state_mutex);
    
    if (context->state == RKLLM_STREAM_STATE_PAUSED) {
        context->state = RKLLM_STREAM_STATE_STREAMING;
        printf("▶️  Resumed streaming session: %s\n", context->session_id);
        pthread_cond_broadcast(&context->state_changed);
    }
    
    pthread_mutex_unlock(&context->state_mutex);
    return 0;
}

int rkllm_stream_abort(rkllm_stream_context_t* context) {
    if (!context) return -1;
    
    pthread_mutex_lock(&context->state_mutex);
    
    context->state = RKLLM_STREAM_STATE_ABORTED;
    printf("🛑 Aborted streaming session: %s\n", context->session_id);
    pthread_cond_broadcast(&context->state_changed);
    
    pthread_mutex_unlock(&context->state_mutex);
    return 0;
}

int rkllm_stream_get_stats(rkllm_stream_context_t* context, char** stats_json) {
    if (!context || !stats_json) {
        return -1;
    }
    
    pthread_mutex_lock(&context->state_mutex);
    
    json_object* stats = json_object_new_object();
    json_object_object_add(stats, "session_id", json_object_new_string(context->session_id));
    json_object_object_add(stats, "request_id", json_object_new_int(context->request_id));
    json_object_object_add(stats, "state", json_object_new_int(context->state));
    json_object_object_add(stats, "total_tokens", json_object_new_int(context->total_tokens));
    json_object_object_add(stats, "tokens_per_second", json_object_new_int(context->tokens_per_second));
    json_object_object_add(stats, "response_length", json_object_new_int(context->response_length));
    
    uint64_t elapsed_ms = get_timestamp_ms() - context->start_timestamp;
    json_object_object_add(stats, "elapsed_ms", json_object_new_int64(elapsed_ms));
    
    json_object_object_add(stats, "buffer_overflow", 
                          json_object_new_boolean(context->token_buffer.overflow));
    json_object_object_add(stats, "buffer_available", 
                          json_object_new_int(context->token_buffer.available_bytes));
    
    if (context->last_error_code != 0) {
        json_object_object_add(stats, "last_error_code", json_object_new_int(context->last_error_code));
        json_object_object_add(stats, "error_message", json_object_new_string(context->error_message));
    }
    
    const char* json_str = json_object_to_json_string(stats);
    *stats_json = strdup(json_str);
    json_object_put(stats);
    
    pthread_mutex_unlock(&context->state_mutex);
    return 0;
}