# Phân Tích Streaming Architecture cho Realtime MCP

## 1. Tình Trạng Hiện Tại

### 1.1 NANO-IO Communication Flow
**NANO Layer** (`src/nano/core/nano/nano.c`):
```
nano_process_message() →
  io_push_request(json_rpc) →
  [Polling loop 60s timeout] →
  io_pop_response() →
  return MCP response
```

**IO Layer** (`src/io/core/io/io_worker.c`):
```
io_worker_thread() →
  queue_pop(request_queue) →
  io_process_request() →
  rkllm_proxy_execute() →
  queue_push(response_queue) →
  [Complete response]
```

### 1.2 RKLLM Callback Location
**Callback được đăng ký tại IO Layer:**
- File: `src/io/mapping/rkllm_proxy/rkllm_operations.c:121`
- Function: `rkllm_init(&llm_handle, &param, rkllm_proxy_global_callback)`
- Callback thực thi: `rkllm_proxy_global_callback()` trong `rkllm_proxy.c:167`

**Callback Flow:**
```
RKLLM NPU → rkllm_proxy_global_callback() → callback_context buffer → Complete text → IO response
```

### 1.3 Queue Usage Pattern
**Thực tế sử dụng queue:**
- NANO push JSON-RPC request vào `g_io_context.request_queue`
- IO Worker pop từ `request_queue`, xử lý, push vào `response_queue`
- NANO poll `response_queue` để lấy complete response
- **Kết luận: Queue hiện tại chỉ hỗ trợ complete response, không phải streaming**

## 2. Streaming Requirements Analysis

### 2.1 Real-time MCP Streaming Spec
Theo `tmp/issues/realtime-mcp.md`:
- **Chunk-based streaming**: Cần gửi từng chunk text khi RKLLM callback sinh ra
- **Non-blocking communication**: NANO không được block chờ complete response
- **Real-time response**: Client nhận chunk ngay lập tức

### 2.2 Architecture Gaps
**Vấn đề hiện tại:**
1. **Synchronous polling**: NANO block 60s chờ complete response
2. **Buffer accumulation**: Callback chỉ append vào buffer, không stream
3. **Single response model**: Queue chỉ hỗ trợ 1 request = 1 response

**Cần thiết:**
1. **Asynchronous callback**: Callback phải notify NANO ngay khi có chunk
2. **Multi-response model**: 1 request = N streaming chunks + 1 final response
3. **Non-blocking NANO**: NANO return immediately, client poll/websocket cho chunks

## 3. Streaming Architecture Design

### 3.1 Modified Queue System
**Current:**
```
request_queue: [request_item]
response_queue: [complete_response]
```

**Proposed:**
```
request_queue: [request_item]
response_queue: [chunk1, chunk2, ..., final_response]
stream_contexts: {request_id: {state, chunks_sent, ...}}
```

### 3.2 Callback Modification
**Current Callback:**
```c
int rkllm_proxy_global_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    // Append to buffer only
    if (result->text) {
        strcat(context->output_buffer + context->current_pos, result->text);
    }
    return 0;
}
```

**Streaming Callback:**
```c
int rkllm_streaming_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    streaming_context_t* ctx = (streaming_context_t*)userdata;
    
    if (result->text && strlen(result->text) > 0) {
        // Create streaming chunk response
        json_object *chunk_response = create_streaming_chunk(ctx->request_id, result->text, false);
        
        // Push chunk to response queue immediately
        queue_item_t chunk_item = {
            .request_id = ctx->request_id,
            .handle_id = ctx->handle_id,
            .method = "stream_chunk",
            .params = json_object_to_json_string(chunk_response),
            .timestamp = time(nullptr)
        };
        queue_push(&g_io_context.response_queue, &chunk_item);
        
        json_object_put(chunk_response);
    }
    
    if (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR) {
        // Create final response
        json_object *final_response = create_streaming_final(ctx->request_id, state);
        
        queue_item_t final_item = {
            .request_id = ctx->request_id,
            .handle_id = ctx->handle_id,
            .method = "stream_final",
            .params = json_object_to_json_string(final_response),
            .timestamp = time(nullptr)
        };
        queue_push(&g_io_context.response_queue, &final_item);
        
        json_object_put(final_response);
        ctx->finished = true;
    }
    
    return 0;
}
```

### 3.3 NANO Layer Streaming
**Current NANO:**
```c
int nano_process_message() {
    io_push_request();
    
    // Block and poll
    while (timeout < 60) {
        if (io_pop_response()) break;
        sleep(1);
    }
    
    return response;
}
```

**Streaming NANO:**
```c
int nano_process_streaming_message() {
    // Push request với streaming flag
    io_push_streaming_request();
    
    // Return immediately với streaming ID
    return create_streaming_response(request_id);
}

// Separate polling function for chunks
int nano_poll_streaming_chunks(uint32_t request_id, chunk_callback_t callback) {
    while (!finished) {
        queue_item_t chunk;
        if (io_pop_response_by_id(request_id, &chunk) == 0) {
            if (strcmp(chunk.method, "stream_chunk") == 0) {
                callback(chunk.params); // Send to client
            } else if (strcmp(chunk.method, "stream_final") == 0) {
                callback(chunk.params);
                break;
            }
        }
        usleep(1000); // 1ms polling
    }
    return 0;
}
```

## 4. Implementation Strategy

### 4.1 Phase 1: Queue System Enhancement
1. **Extend queue_item_t**: Add `stream_id`, `chunk_sequence`, `is_final` fields
2. **Add streaming context**: `streaming_context_t` to track active streams
3. **Modify queue operations**: Support filtering by request_id and stream type

### 4.2 Phase 2: RKLLM Callback Streaming
1. **Create streaming callback**: Replace `rkllm_proxy_global_callback`
2. **Chunk generation**: Immediate push to response queue per callback
3. **Context management**: Track streaming state per request

### 4.3 Phase 3: NANO Streaming Interface
1. **Non-blocking request**: `nano_process_streaming_message()`
2. **Chunk polling**: `nano_poll_streaming_chunks()`
3. **WebSocket integration**: Real-time chunk delivery to client

### 4.4 Phase 4: Transport Layer Integration
1. **WebSocket streaming**: Direct chunk forwarding
2. **HTTP chunked transfer**: For HTTP clients
3. **TCP streaming**: Raw chunk streaming for TCP clients

## 5. Performance Considerations

### 5.1 Memory Management
- **Chunk buffer**: Small fixed-size buffers per chunk (256-512 bytes)
- **Context cleanup**: Auto-cleanup finished streaming contexts
- **Queue capacity**: Increase queue size for multiple concurrent streams

### 5.2 Latency Optimization
- **Immediate callback**: Zero buffering in callback
- **Micro-polling**: 1ms polling interval for real-time
- **Direct queue access**: Bypass worker thread for streaming responses

## 6. Compatibility Assessment

### 6.1 Current System Compatibility
**✅ Có thể tương thích:**
- Queue infrastructure đã có sẵn
- JSON-RPC format có thể extend cho streaming
- Worker thread model có thể adapt

**❌ Cần thay đổi:**
- Callback accumulation logic
- NANO synchronous polling
- Single response assumption

### 6.2 Backward Compatibility
**Giải pháp:**
- Keep existing `rkllm_op_run()` for sync operations
- Add new `rkllm_op_run_streaming()` for async streaming
- Client specify streaming mode in request

## 7. Complete RKLLM API Exposure

### 7.1 Client Access to All RKLLM Functions
**✅ ĐÚNG**: Client có thể gọi **TẤT CẢ** functions của RKLLM library thông qua Nano!

**Available RKLLM Operations cho Client:**

**Functions KHÔNG cần handle_id (utility functions):**
```json
// Create default parameters - không cần handle
{
  "jsonrpc": "2.0", 
  "id": 1,
  "method": "create_default_param",  // rkllm_createDefaultParam()
  "params": {}
}
// Response: {"jsonrpc": "2.0", "id": 1, "result": {"default_params": {...}}}
```

**Functions CẦN handle_id (model operations):**
```json
// Step 1: Initialize model handle - trả về handle_id
{
  "jsonrpc": "2.0", 
  "id": 2,
  "method": "init",           // rkllm_init(LLMHandle* handle, ...)
  "params": {"model_path": "models/qwen3/model.rkllm"}
}
// Response: {"jsonrpc": "2.0", "id": 2, "result": {"handle_id": 123}}

// Step 2: Tất cả operations khác cần handle_id
{
  "jsonrpc": "2.0", 
  "id": 3,
  "method": "run",            // rkllm_run(LLMHandle handle, ...)
  "params": {
    "handle_id": 123,
    "prompt": "Hello, how are you?"
  }
}

{
  "jsonrpc": "2.0", 
  "id": 4,
  "method": "run_async",      // rkllm_run_async(LLMHandle handle, ...)
  "params": {
    "handle_id": 123,
    "prompt": "Tell me a story"
  }
}

{
  "jsonrpc": "2.0",
  "id": 5,
  "method": "load_lora",      // rkllm_load_lora(LLMHandle handle, ...)
  "params": {
    "handle_id": 123,
    "lora_adapter_path": "models/lora/lora.rkllm"
  }
}

{
  "jsonrpc": "2.0",
  "id": 6,
  "method": "set_chat_template",  // rkllm_set_chat_template(LLMHandle handle, ...)
  "params": {
    "handle_id": 123,
    "system_prompt": "You are a helpful assistant",
    "prompt_prefix": "User: ",
    "prompt_postfix": "\nAssistant: "
  }
}

{
  "jsonrpc": "2.0",
  "id": 7,
  "method": "set_function_tools", // rkllm_set_function_tools(LLMHandle handle, ...)
  "params": {
    "handle_id": 123,
    "system_prompt": "You can call functions",
    "tools": "[{\"name\": \"get_weather\", \"parameters\": {...}}]"
  }
}

{
  "jsonrpc": "2.0",
  "id": 8,
  "method": "clear_kv_cache",     // rkllm_clear_kv_cache(LLMHandle handle, ...)
  "params": {
    "handle_id": 123,
    "keep_system_prompt": true
  }
}

{
  "jsonrpc": "2.0", 
  "id": 9,
  "method": "abort",              // rkllm_abort(LLMHandle handle)
  "params": {"handle_id": 123}
}

{
  "jsonrpc": "2.0",
  "id": 10,
  "method": "is_running",         // rkllm_is_running(LLMHandle handle)
  "params": {"handle_id": 123}
}

// Step 3: Cleanup when done
{
  "jsonrpc": "2.0",
  "id": 11, 
  "method": "destroy",            // rkllm_destroy(LLMHandle handle)
  "params": {"handle_id": 123}
}
```

### 7.2 Complete API Mapping với Handle Translation
**NANO = Complete RKLLM Proxy với Handle Management:**
- **15 RKLLM functions** → **15 JSON-RPC methods**
- **Handle translation**: NANO `handle_id` → IO `LLMHandle` (actual pointer)
- **Parameter mapping**: JSON params → RKLLM structs
- **Result formatting**: RKLLM results → JSON responses

**Handle Translation Flow:**
```
Client request: {"method": "run", "params": {"handle_id": 123, "prompt": "hello"}}
     ↓
NANO: Validates request, forwards to IO với handle_id
     ↓  
IO Worker: handle_id → LLMHandle (via handle_pool_get())
     ↓
RKLLM: rkllm_run(LLMHandle, RKLLMInput*, RKLLMInferParam*, userdata)
     ↓
IO Worker: RKLLM result → JSON response  
     ↓
NANO: Forward JSON response to client
```

**Functions BY Category:**
1. **Utility (no handle needed):**
   - `create_default_param` → `rkllm_createDefaultParam()`

2. **Handle lifecycle:**
   - `init` → `rkllm_init(LLMHandle* handle, ...)` - creates handle
   - `destroy` → `rkllm_destroy(LLMHandle handle)` - destroys handle

3. **Model operations (need handle):**
   - `run`, `run_async`, `abort`, `is_running`
   - `load_lora`, `load_prompt_cache`, `release_prompt_cache`  
   - `clear_kv_cache`, `get_kv_cache_size`
   - `set_chat_template`, `set_function_tools`, `set_cross_attn_params`

**Handle Pool Management:**
- **Current**: `MAX_HANDLES = 1` (RK3588 limitation)
- **Pool operations**: `handle_pool_get()`, `handle_pool_set_handle()`, `handle_pool_destroy()`
- **Future**: Increase pool size khi hardware cho phép multiple models

### 7.3 Architecture Benefits
**1. Universal RKLLM Client with Handle Management:**
```
Client → JSON-RPC → NANO → IO Worker → Handle Pool[1] → RKLLM → RK3588 NPU
      ←           ←      ←            ←               ←        ←
```

**Current Limitations (Hardware):**
- **Single model**: Only 1 handle active (RK3588 memory constraint)
- **Single worker**: Only 1 IO worker thread (NPU limitation)
- **Serial processing**: Requests queued and processed sequentially

**Future Scalability (When Hardware Improves):**
- **Multiple models**: `MAX_HANDLES = 8` → 8 concurrent model instances
- **Multiple workers**: `WORKER_COUNT = 4` → 4 parallel inference threads
- **Parallel processing**: Multiple clients can use different models simultaneously

**2. Language Agnostic với Handle Translation:**
```python
# Python client - proper handle lifecycle
# Get default params (no handle needed)
defaults = nano.call("create_default_param", {})

# Initialize model (returns handle_id)  
result = nano.call("init", {"model_path": "qwen3.rkllm"})
handle_id = result["result"]["handle_id"]

# Model operations (need handle_id)
nano.call("set_function_tools", {
    "handle_id": handle_id, 
    "system_prompt": "You can call functions",
    "tools": "[...]"
})

# Inference (need handle_id)
result = nano.call("run", {
    "handle_id": handle_id, 
    "prompt": "Hello"
})

# Cleanup (need handle_id)  
nano.call("destroy", {"handle_id": handle_id})
```

```javascript
// JavaScript client - handle management
// Utility function (no handle)
const defaults = await nano.call("create_default_param", {});

// Initialize (returns handle_id)
const {handle_id} = await nano.call("init", {model_path: "qwen3.rkllm"});

// Model operations (need handle_id)
await nano.call("load_lora", {handle_id, lora_adapter_path: "creative.rkllm"});
await nano.call("set_chat_template", {
    handle_id, 
    system_prompt: "You are creative assistant"
});

// Inference (need handle_id)  
const result = await nano.call("run_async", {handle_id, prompt: "Story"});

// Cleanup (need handle_id)
await nano.call("destroy", {handle_id});
```

**3. Network Accessible NPU with Resource Management:**
- **Handle sharing**: Multiple clients can request same handle_id (managed by pool)
- **Resource cleanup**: Automatic handle destruction on client disconnect
- **Load balancing**: Future support for multiple Nano instances with handle distribution

## 8. Kết Luận

### 8.1 Câu Trả Lời Cho Các Câu Hỏi
1. **"NANO đang thực sự vào queue đầu ra của IO để lấy dữ liệu, hay chỉ là lý thuyết?"**
   - ✅ **THỰC SỰ**: NANO call `io_pop_response()` để lấy từ `g_io_context.response_queue`

2. **"Hàm callback truyền vào cho rkllm để nhận đầu ra của rkllm đang ở IO hay ở NANO?"**
   - ✅ **Ở IO**: Callback `rkllm_proxy_global_callback()` registered trong `rkllm_operations.c`

3. **"Kiến trúc hiện tại của chúng ta đủ để cho IO truyền stream về cho NANO chưa?"**
   - ❌ **CHƯA ĐỦ**: Cần modify callback để push chunks thay vì accumulate buffer
   - ❌ **CHƯA ĐỦ**: Cần extend queue system cho multi-response per request
   - ❌ **CHƯA ĐỦ**: Cần non-blocking NANO interface

4. **"Client có thể gọi bất kỳ hàm nào mà RKLLM có không?"**
   - ✅ **HOÀN TOÀN ĐÚNG**: Client có access đến **15/15 RKLLM functions**
   - 🔧 **Handle translation**: NANO chuyển đổi `handle_id` → `LLMHandle` cho IO
   - � **1 function không cần handle**: `create_default_param`
   - 🔒 **14 functions cần handle**: Tất cả model operations
   - 🚀 **Future**: Sẽ support multiple handles khi hardware cho phép

### 8.2 Current Architecture Constraints
**Hardware Limitations (RK3588):**
- **Single model memory**: NPU chỉ load được 1 model cùng lúc (~932MB Qwen3)
- **Memory bandwidth**: Không đủ để run multiple models parallel
- **Worker limitation**: 1 IO worker thread để avoid NPU conflicts

**Design cho Future Scalability:**
```c
// Current settings (constrained by hardware)
#define MAX_HANDLES 1        // Will increase to 8+ later
#define WORKER_COUNT 1       // Will increase to 4+ later
#define MAX_QUEUE_SIZE 100   // Already ready for high throughput

// Future settings (when hardware improves)
#define MAX_HANDLES 8        // Multiple model instances
#define WORKER_COUNT 4       // Parallel processing
#define MAX_QUEUE_SIZE 1000  // High concurrency support
```

**Client Handle Management Flow:**
```
Client A: init(qwen3) → handle_id=123 → run(123, "hello") → destroy(123)
Client B: [waits] → init(gemma) → handle_id=124 → run(124, "story") → destroy(124)
// Currently sequential due to hardware, future will be parallel
```

### 8.3 Action Items
1. **Immediate**: Implement streaming callback prototype (with handle_id tracking)
2. **Short-term**: Extend queue system for multi-response per handle
3. **Medium-term**: Create NANO streaming interface with handle context
4. **Long-term**: Full WebSocket streaming integration
5. **Future**: Scale handle pool and worker count when hardware allows

### 8.4 Effort Estimation
- **Queue modification**: 2-3 days (add handle_id filtering)
- **Streaming callback**: 1-2 days (maintain handle context)
- **NANO streaming**: 3-4 days (handle-aware streaming)
- **Transport integration**: 5-7 days (handle lifecycle management)
- **Total**: ~2 weeks for complete streaming implementation

**Note**: All streaming implementation will preserve handle management architecture for future scalability.
