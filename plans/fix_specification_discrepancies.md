# Kế hoạch khắc phục sai lệch giữa Codebase và Thiết kế

**Ngày tạo**: 2025-07-21  
**Ưu tiên**: HIGH (P0 - MVP Blocking)  
**Thời gian ước tính**: 4-6 tuần  

## Tóm tắt vấn đề

Sau khi kiểm tra tỉ mỉ, phát hiện **sai lệch nghiêm trọng** giữa codebase và thiết kế (IDEA.md + PRD.md):
- **Streaming thời gian thực**: Hoàn toàn thiếu tích hợp callback → MCP layer
- **HTTP Polling System**: Chưa triển khai method "poll"  
- **Transport Connections**: Chỉ STDIO hoạt động đầy đủ
- **PRD estimates**: Không chính xác (claim 30%, thực tế ~15-20%)

## Phase 1: Core Streaming Implementation (2 tuần)

### **Priority 1.1: RKLLM Callback Integration** 
**Vấn đề**: Callback `rkllm_proxy_callback()` thu thập tokens nhưng không forward đến streaming system

#### **File cần sửa**: `src/lib/core/rkllm_proxy.c:30-82`

**Hiện tại**:
```c
static int rkllm_proxy_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    // Chỉ lưu vào g_response_buffer cục bộ
    if (result->text && strlen(result->text) > 0) {
        strcat(g_response_buffer, result->text);
    }
    // THIẾU: Không forward đến streaming system
    return 0;
}
```

**Cần thêm**:
```c
static int rkllm_proxy_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    // Existing local buffer logic...
    if (result->text && strlen(result->text) > 0) {
        strcat(g_response_buffer, result->text);
        
        // NEW: Forward to streaming system
        if (g_current_stream_session) {
            stream_chunk_t chunk = {
                .seq = g_current_stream_session->chunk_count++,
                .delta = strdup(result->text),
                .end = (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR),
                .error = (state == RKLLM_RUN_ERROR) ? "RKLLM error" : NULL
            };
            
            // Add chunk to ALL active transports
            stream_add_chunk(g_current_stream_session->request_id, &chunk);
        }
    }
    return 0;
}
```

#### **Thêm Session Management**
**File mới**: `src/lib/core/stream_session_tracker.h`
```c
typedef struct {
    char request_id[64];
    char method[64];
    uint32_t seq;
    char delta[2048];
    bool end;
    char* error_message;
} stream_session_t;

// Global tracking
extern stream_session_t* g_current_stream_session;

// Functions
int stream_session_start(const char* request_id, int transport_mask);
void stream_session_end(const char* request_id);
```

### **Priority 1.2: Real-Time Transport Streaming**
**Vấn đề**: Transports nhận chunks nhưng không gửi realtime

#### **STDIO Transport Enhancement**
**File**: `src/lib/transport/stdio.c` - Thêm streaming support
```c
// Thêm callback registration
int stdio_register_stream_callback(stream_callback_fn callback) {
    g_stdio_stream_callback = callback;
    return 0;
}

// Thêm real-time chunk sending
int stdio_send_stream_chunk(const stream_chunk_t* chunk, const char* request_id) {
    json_object* chunk_msg = json_object_new_object();
    json_object_object_add(chunk_msg, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(chunk_msg, "method", json_object_new_string("rkllm_run_async"));
    json_object_object_add(chunk_msg, "id", json_object_new_string(request_id));
    
    json_object* result = json_object_new_object();
    json_object* chunk_obj = json_object_new_object();
    json_object_object_add(chunk_obj, "seq", json_object_new_int(chunk->seq));
    json_object_object_add(chunk_obj, "delta", json_object_new_string(chunk->delta));
    json_object_object_add(chunk_obj, "end", json_object_new_boolean(chunk->end));
    
    json_object_object_add(result, "chunk", chunk_obj);
    json_object_object_add(chunk_msg, "result", result);
    
    const char* json_string = json_object_to_json_string(chunk_msg);
    printf("%s\n", json_string);  // STDIO output
    fflush(stdout);
    
    json_object_put(chunk_msg);
    return 0;
}
```

#### **WebSocket Transport - Real Streaming**
**File**: `src/lib/transport/websocket.c:89-150` - Hoàn thành implementation
```c
int websocket_send_stream_chunk(const stream_chunk_t* chunk, const char* request_id) {
    if (!g_websocket_connected) return -1;
    
    // Format theo IDEA.md specification (dòng 96-110)
    json_object* message = json_object_new_object();
    json_object_object_add(message, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(message, "method", json_object_new_string("rkllm_run_async"));
    json_object_object_add(message, "id", json_object_new_string(request_id));
    
    json_object* result = json_object_new_object();
    json_object* chunk_data = json_object_new_object();
    json_object_object_add(chunk_data, "seq", json_object_new_int(chunk->seq));
    json_object_object_add(chunk_data, "delta", json_object_new_string(chunk->delta));
    json_object_object_add(chunk_data, "end", json_object_new_boolean(chunk->end));
    
    if (chunk->error) {
        json_object_object_add(chunk_data, "error", json_object_new_string(chunk->error));
    }
    
    json_object_object_add(result, "chunk", chunk_data);
    json_object_object_add(message, "result", result);
    
    const char* json_str = json_object_to_json_string(message);
    
    // Send via WebSocket
    wsServer_sendText(g_ws_server, g_client_fd, (char*)json_str, strlen(json_str));
    
    json_object_put(message);
    return 0;
}
```

### **Priority 1.3: HTTP Polling System Implementation**
**Vấn đề**: Method "poll" trả về "not implemented" 

#### **Complete Poll Method**
**File**: `src/lib/protocol/mcp_protocol.c:357-364`

**Hiện tại**:
```c
int mcp_handle_stream_poll_request(const char* request_id, char* response, size_t response_size) {
    snprintf(response, response_size, 
        "{\"error\": \"Stream polling not implemented at protocol level\"}");
    return -1;
}
```

**Cần thay thế**:
```c
int mcp_handle_stream_poll_request(const char* request_id, char* response, size_t response_size) {
    // Find HTTP buffer for this request_id
    http_stream_buffer_t* buffer = http_stream_buffer_find(request_id);
    
    if (!buffer) {
        snprintf(response, response_size,
            "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"error\":{\"code\":-32002,\"message\":\"Stream not found\"}}", 
            request_id);
        return -1;
    }
    
    if (buffer->chunk_count == 0) {
        // No new chunks yet
        snprintf(response, response_size,
            "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"result\":{\"chunks\":[],\"more\":true}}", 
            request_id);
        return 0;
    }
    
    // Build response with accumulated chunks
    json_object* response_obj = json_object_new_object();
    json_object_object_add(response_obj, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(response_obj, "id", json_object_new_string(request_id));
    
    json_object* result = json_object_new_object();
    json_object* chunks_array = json_object_new_array();
    
    // Accumulate all chunks into single "delta" (theo IDEA.md dòng 121)
    char* accumulated_delta = malloc(buffer->total_size + 1);
    accumulated_delta[0] = '\0';
    
    bool is_final = false;
    for (int i = 0; i < buffer->chunk_count; i++) {
        strcat(accumulated_delta, buffer->chunks[i].delta);
        if (buffer->chunks[i].end) {
            is_final = true;
        }
    }
    
    // Create single accumulated chunk (theo IDEA.md dòng 113-126)
    json_object* chunk_obj = json_object_new_object();
    json_object_object_add(chunk_obj, "seq", json_object_new_int(0));
    json_object_object_add(chunk_obj, "delta", json_object_new_string(accumulated_delta));
    json_object_object_add(chunk_obj, "end", json_object_new_boolean(is_final));
    
    json_object_array_add(chunks_array, chunk_obj);
    json_object_object_add(result, "chunk", chunk_obj);  // Single chunk format
    json_object_object_add(response_obj, "result", result);
    
    const char* json_string = json_object_to_json_string(response_obj);
    snprintf(response, response_size, "%s", json_string);
    
    // Clear buffer after polling (theo IDEA.md dòng 52)
    http_stream_buffer_clear(buffer);
    
    if (is_final) {
        http_stream_buffer_remove(request_id);
    }
    
    free(accumulated_delta);
    json_object_put(response_obj);
    return 0;
}
```

#### **HTTP Stream Buffer System**
**File mới**: `src/lib/transport/http_stream_buffer.h`
```c
typedef struct {
    stream_chunk_t* chunks;
    int chunk_count;
    int chunk_capacity;
    size_t total_size;
    time_t last_poll;
    time_t expires_at;
} http_stream_buffer_t;

typedef struct {
    char request_id[64];
    http_stream_buffer_t buffer;
} http_stream_entry_t;

// Global HTTP stream registry
typedef struct {
    http_stream_entry_t* entries;
    int count;
    int capacity;
    pthread_mutex_t mutex;
} http_stream_registry_t;

// Functions
http_stream_buffer_t* http_stream_buffer_find(const char* request_id);
int http_stream_buffer_add_chunk(const char* request_id, const stream_chunk_t* chunk);
void http_stream_buffer_clear(http_stream_buffer_t* buffer);
void http_stream_buffer_cleanup_expired(void);  // Called every 30s
void stream_session_end(const char* request_id);
```

## Phase 2: Transport Layer Completion (1.5 tuần)

### **Priority 2.1: TCP Transport Real Implementation**
**File**: `src/lib/transport/tcp.c:85-150`

**Hiện tại**: Chỉ có skeleton functions

**Cần hoàn thành**:
```c
int tcp_transport_recv(transport_base_t* transport, char* buffer, size_t buffer_size, int timeout_ms) {
    tcp_config_t* config = (tcp_config_t*)transport->config;
    
    if (!config->server_running) {
        // Start server if not running
        if (tcp_start_server(config) != 0) {
            return TRANSPORT_ERROR;
        }
    }
    
    // Accept new connection or use existing
    int client_fd = tcp_accept_client(config, timeout_ms);
    if (client_fd < 0) {
        return TRANSPORT_TIMEOUT;
    }
    
    // Read complete message (JSON-RPC)
    ssize_t received = tcp_read_json_message(client_fd, buffer, buffer_size, timeout_ms);
    
    // Store client for response
    config->current_client = client_fd;
    
    return (received > 0) ? TRANSPORT_OK : TRANSPORT_ERROR;
}

int tcp_transport_send(transport_base_t* transport, const char* data, size_t len) {
    tcp_config_t* config = (tcp_config_t*)transport->config;
    
    if (config->current_client < 0) {
        return TRANSPORT_ERROR;
    }
    
    ssize_t sent = send(config->current_client, data, len, 0);
    
    // Keep connection alive for streaming
    if (!config->keep_alive) {
        close(config->current_client);
        config->current_client = -1;
    }
    
    return (sent == (ssize_t)len) ? TRANSPORT_OK : TRANSPORT_ERROR;
}
```

### **Priority 2.2: UDP Transport Reliability**
**File**: `src/lib/transport/udp.c:89-120`

**Thêm**: Packet ordering và reliability như mô tả trong PRD
```c
typedef struct {
    uint32_t sequence;
    uint32_t total_packets;
    uint32_t packet_id;
    char payload[UDP_MAX_PAYLOAD];
} udp_packet_t;

int udp_send_large_message(const char* data, size_t len, struct sockaddr_in* addr) {
    size_t packets_needed = (len + UDP_MAX_PAYLOAD - 1) / UDP_MAX_PAYLOAD;
    uint32_t sequence = generate_sequence_number();
    
    for (size_t i = 0; i < packets_needed; i++) {
        udp_packet_t packet = {
            .sequence = sequence,
            .total_packets = packets_needed,
            .packet_id = i
        };
        
        size_t chunk_size = min(UDP_MAX_PAYLOAD, len - i * UDP_MAX_PAYLOAD);
        memcpy(packet.payload, data + i * UDP_MAX_PAYLOAD, chunk_size);
        
        // Send with retry
        for (int retry = 0; retry < UDP_MAX_RETRIES; retry++) {
            if (sendto(g_udp_socket, &packet, sizeof(udp_packet_t), 0, 
                      (struct sockaddr*)addr, sizeof(*addr)) > 0) {
                break;
            }
            usleep(UDP_RETRY_DELAY_US);
        }
    }
    
    return 0;
}
```

### **Priority 2.3: WebSocket Connection Lifecycle**
**File**: `src/lib/transport/websocket.c:45-89`

**Hiện tại**: Basic threading, thiếu error handling

**Cần thêm**:
```c
typedef enum {
    WS_STATE_DISCONNECTED,
    WS_STATE_CONNECTING,
    WS_STATE_CONNECTED,
    WS_STATE_ERROR
} ws_connection_state_t;

typedef struct {
    ws_connection_state_t state;
    pthread_t server_thread;
    wsServer* server;
    int client_count;
    int client_fds[MAX_WS_CLIENTS];
    pthread_mutex_t clients_mutex;
    
    // Heartbeat
    time_t last_ping;
    bool heartbeat_enabled;
} ws_connection_manager_t;

// Connection lifecycle
int websocket_handle_connection(int client_fd) {
    pthread_mutex_lock(&g_ws_manager.clients_mutex);
    
    if (g_ws_manager.client_count < MAX_WS_CLIENTS) {
        g_ws_manager.client_fds[g_ws_manager.client_count++] = client_fd;
        printf("WebSocket client connected: fd=%d, total=%d\n", 
               client_fd, g_ws_manager.client_count);
    }
    
    pthread_mutex_unlock(&g_ws_manager.clients_mutex);
    return 0;
}

int websocket_handle_disconnection(int client_fd) {
    pthread_mutex_lock(&g_ws_manager.clients_mutex);
    
    for (int i = 0; i < g_ws_manager.client_count; i++) {
        if (g_ws_manager.client_fds[i] == client_fd) {
            // Remove client
            memmove(&g_ws_manager.client_fds[i], 
                   &g_ws_manager.client_fds[i + 1],
                   (g_ws_manager.client_count - i - 1) * sizeof(int));
            g_ws_manager.client_count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_ws_manager.clients_mutex);
    return 0;
}
```

## Phase 3: Data Flow Compliance (1 tuần)

### **Priority 3.1: Complete 9-Step Data Flow**
**Theo IDEA.md dòng 130-138**

#### **Step 6-7 Implementation**
**File**: `src/lib/core/stream_buffer_manager.c` (mới)
```c
int buffer_manager_process_token(const char* token, const char* request_id, 
                               int transport_mask, bool is_final) {
    // Step 6: RKLLM lib calls callback → Buffer manager ✓
    
    // Step 7: Buffer manager check transport type
    for (int i = 0; i < TRANSPORT_COUNT; i++) {
        if (!(transport_mask & (1 << i))) continue;
        
        switch (i) {
            case TRANSPORT_HTTP: {
                // HTTP: Keep joining tokens until client polls (IDEA.md dòng 47)
                http_stream_buffer_add_chunk(request_id, &(stream_chunk_t){
                    .delta = strdup(token),
                    .end = is_final
                });
                break;
            }
            
            case TRANSPORT_STDIO:
            case TRANSPORT_TCP:
            case TRANSPORT_UDP:
            case TRANSPORT_WEBSOCKET: {
                // Real-time: Pass data to MCP layer immediately (IDEA.md dòng 40)
                mcp_send_stream_chunk(i, request_id, &(stream_chunk_t){
                    .seq = get_next_sequence(request_id),
                    .delta = strdup(token),
                    .end = is_final
                });
                break;
            }
        }
    }
    
    return 0;
}
```

#### **Step 8-9: MCP Layer Packing + Transport Send**
**File**: `src/lib/protocol/mcp_protocol.c` - Thêm function
```c
int mcp_send_stream_chunk(int transport_id, const char* request_id, const stream_chunk_t* chunk) {
    // Step 8: MCP layer packs message to be MCP compliant
    json_object* message = json_object_new_object();
    json_object_object_add(message, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(message, "method", json_object_new_string("rkllm_run_async"));  // Original method
    json_object_object_add(message, "id", json_object_new_string(request_id));
    
    json_object* result = json_object_new_object();
    json_object* chunk_obj = json_object_new_object();
    json_object_object_add(chunk_obj, "seq", json_object_new_int(chunk->seq));
    json_object_object_add(chunk_obj, "delta", json_object_new_string(chunk->delta));
    json_object_object_add(chunk_obj, "end", json_object_new_boolean(chunk->end));
    
    json_object_object_add(result, "chunk", chunk_obj);
    json_object_object_add(message, "result", result);
    
    const char* json_string = json_object_to_json_string(message);
    
    // Step 9: Transport sends MCP compliant JSON back to client
    transport_base_t* transport = get_transport_by_id(transport_id);
    if (transport && transport->send_raw) {
        transport->send_raw(transport, json_string, strlen(json_string));
    }
    
    json_object_put(message);
    return 0;
}
```

### **Priority 3.2: Connection Integration**
**File**: `src/lib/core/rkllm_proxy.c:1164-1174` - Callback integration

**Hiện tại**: Callback isolated
```c
static int rkllm_proxy_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    // Only local buffering...
}
```

**Cần thêm**: Complete integration
```c
// Global state tracking
static char g_current_request_id[64] = {0};
static int g_current_transport_mask = 0;

static int rkllm_proxy_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    // Existing local buffer logic (keep for compatibility)...
    
    // NEW: Real-time streaming integration
    if (result->text && strlen(result->text) > 0 && g_current_request_id[0] != '\0') {
        bool is_final = (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR);
        
        // Call buffer manager (Step 6-7 from IDEA.md)
        buffer_manager_process_token(result->text, g_current_request_id, 
                                   g_current_transport_mask, is_final);
    }
    
    // Clean up on completion
    if (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR) {
        memset(g_current_request_id, 0, sizeof(g_current_request_id));
        g_current_transport_mask = 0;
    }
    
    return 0;
}
```

## Phase 4: Settings & Configuration Fixes (0.5 tuần)

### **Priority 4.1: Fix 3-Tier Priority System**
**File**: `src/lib/core/settings.c:89-150`

**Vấn đề**: Không có evidence của 3-tier priority

**Cần thêm**:
```c
typedef struct {
    const char* cli_value;      // Highest priority
    const char* settings_value; // Medium priority  
    const char* default_value;  // Lowest priority
} setting_priority_t;

const char* resolve_setting_priority(const setting_priority_t* setting) {
    if (setting->cli_value) return setting->cli_value;
    if (setting->settings_value) return setting->settings_value;
    return setting->default_value;
}

// Apply to all settings loading
void load_settings_with_priority(mcp_settings_t* settings, int argc, char* argv[]) {
    // 1. Load defaults
    apply_default_settings(settings);
    
    // 2. Override with settings.json
    if (file_exists("settings.json")) {
        load_settings_from_json(settings, "settings.json");
    }
    
    // 3. Override with CLI arguments (highest priority)
    parse_cli_arguments(settings, argc, argv);
}
```

## Timeline tổng thể

### **Tuần 1-2: Core Streaming (Highest Priority)**
- Ngày 1-3: RKLLM callback integration  
- Ngày 4-7: Real-time transport streaming
- Ngày 8-10: HTTP polling system implementation
- Ngày 11-14: Integration testing và bug fixes

### **Tuần 3-4: Transport Completion**
- Ngày 15-17: TCP transport real implementation
- Ngày 18-19: UDP reliability features  
- Ngày 20-21: WebSocket connection lifecycle
- Ngày 22-24: Cross-transport testing

### **Tuần 5: Data Flow & Settings**
- Ngày 25-26: Complete 9-step data flow
- Ngày 27-28: Settings priority system
- Ngày 29: Integration testing
- Ngày 30: Documentation updates

### **Tuần 6: Validation & Polish**
- Ngày 31-32: Full system testing
- Ngày 33-34: Performance optimization
- Ngày 35-36: Bug fixes và polish

## Success Metrics

### **Functional Tests**
- ✅ **Real-time streaming**: Token flow từ RKLLM → Client qua tất cả transports
- ✅ **HTTP polling**: Client có thể poll và nhận accumulated chunks
- ✅ **Transport stability**: Tất cả 5 transports hoạt động đồng thời
- ✅ **Data flow compliance**: 9 bước theo IDEA.md hoạt động đúng

### **Performance Tests**  
- **Streaming latency**: <10ms per chunk (real-time transports)
- **HTTP polling response**: <50ms cho poll requests
- **Concurrent clients**: 10+ clients streaming đồng thời
- **Memory usage**: Không memory leaks trong streaming

### **Compliance Tests**
- **Chunk format**: Đúng theo IDEA.md specification
- **JSON-RPC**: Đúng chuẩn 2.0 protocol
- **Transport behavior**: Khác biệt đúng giữa real-time vs polling
- **Settings system**: 3-tier priority hoạt động chính xác

## Kết luận

Sau khi hoàn thành kế hoạch này:

1. **Streaming system**: Sẽ hoạt động đúng như thiết kế trong IDEA.md
2. **Transport layer**: Tất cả 5 transports sẽ stable và đầy đủ chức năng  
3. **Data flow**: 9-step flow sẽ comply 100% với specification
4. **PRD accuracy**: Project completion sẽ tăng từ ~15% lên **60-70%**

**Kết quả cuối**: Hệ thống sẽ match hoàn toàn với thiết kế ban đầu và ready cho Phase 2 (Production features) trong PRD.md.