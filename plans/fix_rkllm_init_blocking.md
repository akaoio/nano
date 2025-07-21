# Kế hoạch khắc phục vấn đề Blocking của rkllm_init

**Ngày tạo**: 2025-07-21  
**Ưu tiên**: CRITICAL (P0)  
**Thời gian ước tính**: 3-5 ngày  

## Tóm tắt vấn đề

`rkllm_init` tải model 900MB+ trong 30-60 giây, gây blocking toàn bộ event loop server. Điều này khiến:
- HTTP connections timeout trước khi nhận được response  
- Server trở nên unresponsive hoàn toàn
- Tất cả transport khác bị đình trệ
- 85.7% tests thất bại do server crash/hang

## Phân tích nguyên nhân gốc rễ

### **Root Cause**: Synchronous Architecture
```c
// File: src/lib/core/server.c:155-202
int mcp_server_internal_run_event_loop(mcp_server_internal_t* server) {
    while (server->running) {
        // Event loop đồng bộ - mỗi request xử lý tuần tự
        if (result == TRANSPORT_MANAGER_OK) {
            mcp_server_internal_process_request(server, request, response, size);
            // ↑ Dòng này block 30-60s khi gọi rkllm_init
        }
    }
}
```

### **Impact Chain**:
1. Client gửi `rkllm_init` request
2. Event loop gọi `io_process_operation()` 
3. RKLLM proxy gọi `rkllm_init()` (30-60s)
4. HTTP client timeout và disconnect
5. Server cố gửi response đến connection đã đóng
6. Server trở nên unstable

## Giải pháp: Single-Instance NPU Queue Management

### **Chiến lược tổng quan**
**Hardware constraint**: Rockchip NPU chỉ cho phép 1 instance RKLLM - không thể chạy song song
- **NPU Queue System**: Queue cho 3-5 functions cần NPU memory
- **Instant Processing**: 15+ functions khác xử lý ngay lập tức
- **Operation Classification**: Phân loại dựa trên NPU requirements

### **Phase 1: Background Processing Framework**

#### 1.1 Tạo NPU Operation Classification System
**File mới**: `src/lib/core/npu_operation_classifier.h`
```c
typedef enum {
    OPERATION_INSTANT,      // No NPU - xử lý ngay lập tức
    OPERATION_NPU_QUEUE,    // Cần NPU - queue vào single queue
    OPERATION_STREAMING     // Async streaming với NPU
} npu_operation_type_t;

// Phân loại dựa trên NPU requirements
typedef struct {
    const char* method_name;
    npu_operation_type_t type;
    bool requires_npu_memory;
    int estimated_duration_ms;
} npu_operation_meta_t;

static const npu_operation_meta_t g_npu_operation_registry[] = {
    // Instant processing - No NPU memory needed (15+ functions)
    {"rkllm_get_functions", OPERATION_INSTANT, false, 10},
    {"rkllm_get_constants", OPERATION_INSTANT, false, 5},
    {"rkllm_createDefaultParam", OPERATION_INSTANT, false, 1},
    {"rkllm_destroy", OPERATION_INSTANT, false, 100},
    {"rkllm_abort", OPERATION_INSTANT, false, 50},
    {"rkllm_is_running", OPERATION_INSTANT, false, 1},
    {"rkllm_clear_kv_cache", OPERATION_INSTANT, false, 20},
    {"rkllm_get_kv_cache_size", OPERATION_INSTANT, false, 5},
    {"rkllm_set_chat_template", OPERATION_INSTANT, false, 10},
    {"rkllm_set_function_tools", OPERATION_INSTANT, false, 15},
    {"rkllm_set_cross_attn_params", OPERATION_INSTANT, false, 10},
    {"rkllm_release_prompt_cache", OPERATION_INSTANT, false, 50},
    
    // NPU Queue - Requires exclusive NPU memory access (3-5 functions)
    {"rkllm_init", OPERATION_NPU_QUEUE, true, 45000},               // 45s - Model loading
    {"rkllm_run", OPERATION_NPU_QUEUE, true, 5000},                 // 5s - Synchronous inference
    {"rkllm_run_async", OPERATION_STREAMING, true, -1},             // Streaming inference
    {"rkllm_load_lora", OPERATION_NPU_QUEUE, true, 2000},           // 2s - LoRA adapter loading
    {"rkllm_load_prompt_cache", OPERATION_NPU_QUEUE, true, 1000},   // 1s - Prompt cache loading (uncertain)
};
```

#### 1.2 Single NPU Queue System
**File mới**: `src/lib/core/npu_queue.h`
```c
typedef struct {
    char* method;
    char* params_json;
    char request_id[64];
    int transport_index;
    void* connection_handle;
    time_t queued_at;
    npu_operation_type_t op_type;
} npu_task_t;

typedef struct {
    pthread_t worker_thread;        // Single worker thread (NPU constraint)
    
    // Task queue for NPU operations only
    npu_task_t* task_queue;
    int queue_size;
    int queue_head, queue_tail;
    
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    
    // NPU status tracking
    volatile bool npu_busy;
    volatile bool running;
    char current_operation[64];
    time_t operation_started_at;
} npu_queue_t;

// Functions
int npu_queue_init(npu_queue_t* queue);
int npu_queue_add_task(npu_queue_t* queue, npu_task_t* task);
bool npu_queue_is_busy(npu_queue_t* queue);
void npu_queue_shutdown(npu_queue_t* queue);

// Classification helper
npu_operation_type_t npu_classify_operation(const char* method_name);
```

#### 1.3 Async Response System
**File mới**: `src/lib/core/async_response.h`
```c
typedef struct {
    char request_id[64];
    int transport_index;
    void* connection_handle;
    char* result_json;
    bool completed;
    bool error;
    time_t started_at;
    time_t completed_at;
} async_response_t;

// Response registry cho các async operations
typedef struct {
    async_response_t* responses;
    int capacity;
    int count;
    pthread_mutex_t mutex;
} async_response_registry_t;
```

### **Phase 2: Event Loop Modification**

#### 2.1 Modified Request Processing  
**File**: `src/lib/core/server.c:185`
```c
int mcp_server_internal_process_request(mcp_server_internal_t* server, 
                                      const char* raw_request, 
                                      char* response, 
                                      size_t response_size) {
    // Parse request
    mcp_request_t request;
    mcp_adapter_parse_request(raw_request, &request);
    
    // Classify operation based on NPU requirements
    npu_operation_type_t op_type = npu_classify_operation(request.method);
    
    switch (op_type) {
        case OPERATION_INSTANT: {
            // Process immediately - No NPU needed (15+ functions)
            return mcp_adapter_process_request(&request, &response);
        }
        
        case OPERATION_NPU_QUEUE: {
            // Queue for NPU processing (3-5 functions only)
            npu_task_t task = {
                .method = strdup(request.method),
                .params_json = strdup(request.params),
                .transport_index = server->current_transport_index,
                .connection_handle = server->current_connection,
                .op_type = op_type
            };
            strcpy(task.request_id, request.request_id);
            
            npu_queue_add_task(&server->npu_queue, &task);
            
            // Return immediate "queued" response
            snprintf(response, response_size, 
                "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"result\":{\"status\":\"queued\",\"message\":\"Processing in NPU queue\",\"estimated_wait_ms\":%d}}", 
                request.request_id, get_estimated_wait_time(request.method));
            
            return 0;
        }
        
        case OPERATION_STREAMING: {
            // Handle streaming operations (also uses NPU but different flow)
            return handle_streaming_npu_operation(&request, response, response_size);
        }
    }
}
```

#### 2.2 Single NPU Worker Implementation
**File mới**: `src/lib/core/npu_worker.c`
```c
void* npu_worker_thread(void* arg) {
    npu_queue_t* queue = (npu_queue_t*)arg;
    
    while (queue->running) {
        pthread_mutex_lock(&queue->queue_mutex);
        
        // Wait for NPU tasks
        while (queue->queue_head == queue->queue_tail && queue->running) {
            pthread_cond_wait(&queue->queue_cond, &queue->queue_mutex);
        }
        
        if (!queue->running) break;
        
        // Get NPU task
        npu_task_t task = queue->task_queue[queue->queue_head];
        queue->queue_head = (queue->queue_head + 1) % queue->queue_size;
        
        // Mark NPU as busy
        queue->npu_busy = true;
        strncpy(queue->current_operation, task.method, sizeof(queue->current_operation) - 1);
        queue->operation_started_at = time(NULL);
        
        pthread_mutex_unlock(&queue->queue_mutex);
        
        printf("🔄 NPU Worker: Starting %s (Request ID: %s)\n", task.method, task.request_id);
        
        // Process NPU task (SINGLE INSTANCE ONLY)
        char* result_json = NULL;
        int status = io_process_operation(task.method, task.params_json, &result_json);
        
        printf("✅ NPU Worker: Completed %s in %ld seconds\n", 
               task.method, time(NULL) - queue->operation_started_at);
        
        // Store result
        async_response_t response = {
            .completed = true,
            .error = (status != 0),
            .result_json = result_json,
            .completed_at = time(NULL),
            .started_at = queue->operation_started_at
        };
        strcpy(response.request_id, task.request_id);
        response.transport_index = task.transport_index;
        response.connection_handle = task.connection_handle;
        
        async_response_registry_add(&g_response_registry, &response);
        
        // Mark NPU as free
        pthread_mutex_lock(&queue->queue_mutex);
        queue->npu_busy = false;
        memset(queue->current_operation, 0, sizeof(queue->current_operation));
        pthread_mutex_unlock(&queue->queue_mutex);
        
        // Cleanup
        free(task.method);
        free(task.params_json);
    }
    
    return NULL;
}
```

### **Phase 3: Transport-Specific Enhancements**

#### 3.1 HTTP Keep-Alive + Status Polling
**File**: `src/lib/transport/http.c` - Thêm endpoint mới
```c
// Thêm method "/status/<request_id>" 
int http_handle_status_request(const char* request_id, char* response, size_t response_size) {
    async_response_t* async_response = async_response_registry_find(&g_response_registry, request_id);
    
    if (!async_response) {
        snprintf(response, response_size, 
            "{\"status\":\"not_found\",\"error\":\"Request ID not found\"}");
        return -1;
    }
    
    if (!async_response->completed) {
        time_t elapsed = time(NULL) - async_response->started_at;
        snprintf(response, response_size, 
            "{\"status\":\"processing\",\"elapsed_seconds\":%ld}", elapsed);
        return 0;
    }
    
    if (async_response->error) {
        snprintf(response, response_size, 
            "{\"status\":\"error\",\"error\":\"%s\"}", 
            async_response->result_json ? async_response->result_json : "Unknown error");
        return -1;
    }
    
    // Success - return result and cleanup
    snprintf(response, response_size, 
        "{\"status\":\"completed\",\"result\":%s}", 
        async_response->result_json);
    
    async_response_registry_remove(&g_response_registry, request_id);
    return 0;
}
```

#### 3.2 Connection Management
**File**: `src/lib/transport/connection_manager.h`
```c
typedef struct {
    char request_id[64];
    int socket_fd;
    time_t created_at;
    bool keep_alive;
} pending_connection_t;

// Maintain connections for async operations
int connection_manager_register(const char* request_id, int socket_fd);
int connection_manager_send_response(const char* request_id, const char* response);
```

### **Phase 4: Progressive Enhancement**

#### 4.1 Week 1: NPU Queue Infrastructure
- [ ] Triển khai NPU operation classifier (3-5 functions vs 15+ instant functions)
- [ ] Tạo single NPU queue system (không phải thread pool)
- [ ] Implement async response registry  
- [ ] Modify event loop cho instant operations

#### 4.2 Week 2: NPU Queue Processing  
- [ ] Complete single NPU worker thread
- [ ] Implement result storage and retrieval
- [ ] Add connection management for queued operations
- [ ] Test với rkllm_init trong NPU queue

#### 4.3 Week 3: Transport Enhancements
- [ ] HTTP status polling endpoints cho NPU operations
- [ ] WebSocket progress notifications cho NPU tasks
- [ ] Connection keep-alive management
- [ ] Error handling và timeout cho NPU queue

## Metrics thành công

### **Performance Targets**
- **Instant operations** (15+ functions): 0% blocking time, <10ms response
- **NPU operations** (3-5 functions): Queue processing với status updates
- **HTTP response time**: <50ms cho "queued" response
- **Connection stability**: 0% connection drops do timeout
- **NPU utilization**: Single instance, no crashes từ multiple access

### **Test Scenarios**
1. **Concurrent instant operations**: 10 client đồng thời gọi get_functions, createDefaultParam
2. **Mixed workload**: 5 client gọi instant ops + 1 client gọi rkllm_init trong NPU queue
3. **NPU operation status**: HTTP client theo dõi rkllm_init progress trong queue
4. **Sequential NPU operations**: Multiple rkllm_init, rkllm_run, load_lora requests
5. **Stress test**: 100 instant requests + 5 NPU requests đồng thời

### **Success Criteria**
- ✅ rkllm_init không block server (chạy trong NPU queue)
- ✅ Instant operations hoạt động trong khi NPU operations đang queue
- ✅ HTTP connections không bị timeout
- ✅ Only 1 NPU operation chạy tại 1 thời điểm (hardware constraint)
- ✅ 100% test pass rate (thay vì 14.3% hiện tại)

## Rủi ro và giảm thiểu

### **Technical Risks**
1. **NPU memory conflicts**: Multiple RKLLM calls cannot run simultaneously
   - **Mitigation**: Single NPU queue with exclusive access

2. **Single point of failure**: Only one NPU worker thread
   - **Mitigation**: Robust error handling and automatic recovery

3. **Connection lifecycle**: HTTP connections có thể drop during long NPU operations
   - **Mitigation**: Connection pooling, heartbeat, status polling

### **Implementation Risks**
1. **Queue overflow**: Too many NPU operations queued
   - **Mitigation**: Queue size limits, error responses for overflow

2. **NPU operation timeout**: rkllm_init có thể take longer than expected
   - **Mitigation**: Configurable timeouts, progress monitoring

## Timeline chi tiết

### **Tuần 1** (Ngày 1-5): Foundation
- Ngày 1-2: NPU operation classifier (3-5 vs 15+ functions)
- Ngày 3-4: Single NPU queue system implementation
- Ngày 5: Basic async response system

### **Tuần 2** (Ngày 6-10): Integration  
- Ngày 6-7: Event loop modification cho instant vs NPU queue
- Ngày 8-9: Connection management cho NPU operations
- Ngày 10: Testing với rkllm_init queue

### **Tuần 3** (Ngày 11-15): Enhancement
- Ngày 11-12: HTTP status endpoints cho NPU queue
- Ngày 13-14: Progress monitoring và timeout handling
- Ngày 15: Full integration testing với NPU constraints

**Total**: 15 ngày → **3 tuần hoàn thành**

## Kết luận

Giải pháp này sẽ **hoàn toàn khắc phục** vấn đề blocking của rkllm_init với hardware constraint của Rockchip NPU. Server sẽ trở nên:

- **Responsive**: Instant operations (15+ functions) luôn <10ms
- **NPU-Safe**: Single instance queue ngăn NPU memory conflicts
- **Stable**: Không còn connection timeouts từ blocking operations
- **Hardware-Compliant**: Tuân thủ NPU single-instance limitation
- **Production-ready**: Đạt 99%+ test pass rate

**Key insight**: Instead của thread pool approach, single NPU queue management ensures hardware compatibility while maintaining responsiveness cho majority operations (15+ instant functions).