# PHÂN TÍCH CHUYÊN SÂU VỀ KIẾN TRÚC HỆ THỐNG VÀ QUẢN LÝ BỘ NHỚ NPU

## TÓM TẮT VẤN ĐỀ

Dựa trên phân tích chi tiết kiến trúc hệ thống và thí nghiệm với RKLLM, chúng ta đối mặt với một thách thức cơ bản: **kiến trúc hiện tại được thiết kế để hỗ trợ đa mô hình song song, nhưng phần cứng RK3588 và thư viện RKLLM có giới hạn nghiêm trọng về bộ nhớ NPU**.

## PHÂN TÍCH KIẾN TRÚC HIỆN TẠI

### 1. Kiến Trúc Đa Tầng (Over-engineered)

Hệ thống hiện tại được thiết kế với 4 tầng chính:

```
Application Layer
      ↓
IO Operations (JSON-RPC Processing)  
      ↓
IO Core (Worker Pool + Queue Management)
      ↓
RKLLM Proxy (Handle Pool + Operations Mapping)
      ↓
RKLLM Library (Hardware Abstraction)
```

#### 1.1 IO Operations Layer
- **File**: `src/io/operations.c/h`
- **Chức năng**: Xử lý JSON-RPC request/response, routing đến RKLLM operations
- **Thiết kế**: Stateless, tập trung vào parsing và formatting
- **Vấn đề**: Không cần thiết cho single-model scenario

#### 1.2 IO Core Layer  
- **File**: `src/io/core/io/io.c/h`
- **Chức năng**: Queue management và worker pool
- **Thiết kế**: 
  - Request/Response queues với thread-safe operations
  - Worker pool với `MAX_WORKERS = 5` threads
  - Timeout handling (30 giây)
- **Vấn đề**: Worker pool vô nghĩa khi chỉ có thể chạy 1 model

#### 1.3 RKLLM Proxy Layer
- **File**: `src/io/mapping/rkllm_proxy/rkllm_proxy.c/h`
- **Chức năng**: Abstract hóa RKLLM API, quản lý LLMHandle
- **Thiết kế**: 
  - Handle pool với `MAX_HANDLES = 8` slots
  - Operation mapping (15 operations)
  - Callback context management
- **Vấn đề**: Handle pool phức tạp cho single-model

#### 1.4 Handle Pool Management
- **File**: `src/io/mapping/handle_pool/handle_pool.c/h`
- **Chức năng**: Quản lý LLMHandle instances
- **Thiết kế**:
  ```c
  typedef struct {
      uint32_t id;
      LLMHandle handle;
      bool active;
      char model_path[MAX_MODEL_PATH];
      size_t memory_usage;
      uint64_t last_used;
  } handle_slot_t;
  ```
- **Vấn đề**: Chỉ slot đầu tiên được sử dụng thực tế

### 2. Vấn Đề Cốt Lõi: Giới Hạn Bộ Nhớ NPU

#### 2.1 Thực Tế Phần Cứng RK3588
1. **NPU Memory Pool**: ~6GB shared memory cho NPU
2. **Model Size Impact**:
   - Model nhỏ (500MB-1GB): Load được 2-3 models
   - Model lớn (2-4GB): Chỉ load được 1 model
   - Buffer overhead: ~1-2GB per model
3. **Memory Corruption**: Khi load model thứ 2 và vượt quá memory, toàn bộ NPU bị crash

#### 2.2 Hành Vi RKLLM Library
1. **Automatic Memory Management**: 
   - `rkllm_init()` tự động cấp phát NPU memory
   - Không có API để kiểm soát memory allocation
   - Không có API để query available memory
2. **Memory Leak Risk**:
   - Model thứ 2 load thất bại nhưng model đầu vẫn bị corrupt
   - Cần `rkllm_destroy()` để cleanup memory hoàn toàn
3. **Concurrency Issues**:
   - RKLLM không thread-safe cho multiple handles
   - Race condition khi multiple workers truy cập NPU

## PHÂN TÍCH CHI TIẾT CÁC THÀNH PHẦN

### 1. Worker Pool Analysis

```c
// src/io/core/io/io.c
void* io_worker_thread(void* arg) {
    while (atomic_load(&g_io_context.running)) {
        queue_item_t item;
        if (queue_pop(&g_io_context.request_queue, &item) != 0) {
            usleep(1000); // Busy waiting - inefficient
            continue;
        }
        // Process request...
    }
}
```

**Vấn đề**:
- 5 worker threads cùng competing cho NPU resource
- NPU chỉ có thể xử lý 1 request tại 1 thời điểm  
- Busy waiting gây lãng phí CPU
- Race condition khi multiple threads gọi RKLLM APIs

### 2. Handle Pool Complexity

```c
// src/io/mapping/handle_pool/handle_pool.c
uint32_t handle_pool_create(handle_pool_t* pool, const char* model_path) {
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (!pool->slots[i].active) {
            // Allocate new slot
            pool->slots[i].id = pool->next_id++;
            pool->slots[i].active = true;
            // ...
            return pool->slots[i].id;
        }
    }
    return 0; // No available slots
}
```

**Vấn đề**:
- Chỉ sử dụng 1 slot trong thực tế
- ID mapping overhead không cần thiết
- Memory tracking không chính xác (RKLLM che giấu actual memory usage)

### 3. JSON-RPC Processing Overhead

```c
// src/io/operations.c
int io_process_request(const char* json_request, char* json_response, size_t max_response_len) {
    // Parse JSON-RPC
    uint32_t request_id, handle_id;
    char method[256], params[2048];
    
    if (io_parse_json_request(json_request, &request_id, &handle_id, method, params) != 0) {
        // Error handling with JSON-RPC format
    }
    
    // Get operation enum
    rkllm_operation_t operation = rkllm_proxy_get_operation_by_name(method);
    
    // Create RKLLM request
    rkllm_request_t rkllm_request = {
        .operation = operation,
        .handle_id = handle_id,
        .params_json = params,
        .params_size = strlen(params)
    };
    
    // Execute and format response
    // ...
}
```

**Vấn đề**:
- Multiple JSON parsing/formatting steps
- String operations overhead cho simple function calls
- Error handling duplication ở nhiều layers

## GIẢI PHÁP VÀ KIẾN NGHỊ

### 1. Kiến Trúc Đơn Giản Hóa (Recommended)

#### Option A: Single-Model Architecture
```
Application
    ↓
Direct RKLLM Interface
    ↓
RKLLM Library
```

**Implementation**:
```c
// Simplified interface
typedef struct {
    LLMHandle handle;
    bool initialized;
    char model_path[512];
    pthread_mutex_t mutex;  // Serialize access
} simple_llm_context_t;

int simple_llm_init(const char* model_path);
int simple_llm_run(const char* prompt, char* response, size_t max_len);
void simple_llm_shutdown(void);
```

#### Option B: Single-Worker Architecture
Giữ nguyên kiến trúc nhưng giảm xuống:
- 1 worker thread duy nhất
- 1 handle pool slot
- Simplified queue (optional)

### 2. Memory Management Strategy

#### 2.1 Proactive Model Unloading
```c
int managed_model_switch(const char* new_model_path) {
    // 1. Unload current model if exists
    if (current_handle) {
        rkllm_destroy(current_handle);
        current_handle = nullptr;
    }
    
    // 2. Load new model
    RKLLMParam param = rkllm_createDefaultParam();
    param.model_path = new_model_path;
    
    int ret = rkllm_init(&current_handle, &param, callback);
    if (ret != 0) {
        // Handle initialization failure
        return -1;
    }
    
    return 0;
}
```

#### 2.2 Memory Monitoring
```c
// Estimate memory usage based on model file size
size_t estimate_model_memory(const char* model_path) {
    struct stat st;
    if (stat(model_path, &st) == 0) {
        // Model size + buffer (estimate 50% overhead)
        return st.st_size + (st.st_size / 2);
    }
    return 0;
}

bool can_load_model(const char* model_path) {
    size_t estimated = estimate_model_memory(model_path);
    size_t available = get_available_npu_memory();  // Need to implement
    return estimated <= available;
}
```

### 3. Error Handling & Recovery

#### 3.1 Graceful Degradation
```c
int robust_model_init(const char* model_path) {
    // Try to load model
    int ret = rkllm_init(&handle, &param, callback);
    
    if (ret != 0) {
        // Clear any corrupted state
        if (handle) {
            rkllm_destroy(handle);
            handle = nullptr;
        }
        
        // Reset NPU if possible
        // system("echo 1 > /sys/class/devfreq/fdab0000.npu/reset");
        
        return -1;
    }
    
    return 0;
}
```

#### 3.2 Health Monitoring
```c
typedef struct {
    uint64_t last_successful_inference;
    uint32_t consecutive_failures;
    bool npu_healthy;
} health_status_t;

bool check_npu_health(void) {
    // Try simple inference
    RKLLMInput test_input = {
        .input_type = RKLLM_INPUT_PROMPT,
        .prompt_input = "test"
    };
    
    RKLLMInferParam test_param = {0};
    int ret = rkllm_run(current_handle, &test_input, &test_param, nullptr);
    
    return ret == 0;
}
```

## KẾT LUẬN VÀ KHUYẾN NGHỊ

### 1. Immediate Actions
1. **Giảm xuống 1 worker thread** để tránh race condition
2. **Implement model switching** thay vì parallel loading
3. **Add memory estimation** trước khi load model
4. **Improve error handling** để tránh NPU crash

### 2. Long-term Architecture
1. **Simplify to single-model architecture** nếu hardware limitation không thay đổi
2. **Keep flexibility** để scale up khi RKLLM library cải thiện
3. **Add memory monitoring APIs** khi vendor cung cấp

### 3. Code Changes Priority
1. **High Priority**: Giảm MAX_WORKERS xuống 1
2. **High Priority**: Implement proactive model unloading  
3. **Medium Priority**: Simplify handle pool
4. **Low Priority**: Remove unnecessary abstraction layers

Kiến trúc hiện tại không sai về mặt thiết kế, nhưng **over-engineered** so với hardware constraints. Cần balance giữa flexibility và performance dựa trên thực tế phần cứng.
