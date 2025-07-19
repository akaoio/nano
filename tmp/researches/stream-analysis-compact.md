# Streaming Architecture Analysis - Compact Version

## 1. Current State

### NANO-IO Flow
```
Client → NANO → IO Worker → RKLLM → NPU
      ←      ←            ←       ←
```

**Key Findings:**
- ✅ NANO thực sự dùng queue: `io_pop_response()` từ `g_io_context.response_queue`
- ✅ Callback ở IO layer: `rkllm_proxy_global_callback()` in `rkllm_operations.c:121`
- ❌ Architecture chưa đủ cho streaming: callback chỉ accumulate buffer

## 2. RKLLM API Exposure

**Complete API Access (15/15 functions):**
- **No handle_id needed**: `create_default_param`
- **Need handle_id**: All other 14 functions (init returns handle_id, others use it)

**Client Example:**
```json
{"method": "init", "params": {"model_path": "qwen3.rkllm"}}
→ {"result": {"handle_id": 123}}

{"method": "run", "params": {"handle_id": 123, "prompt": "hello"}}
→ {"result": {"data": "Hello! How are you?"}}
```

## 3. Streaming Gap Analysis

### Current Issues
1. **Sync polling**: NANO blocks 60s for complete response
2. **Buffer accumulation**: Callback stores all text, no real-time chunks
3. **1:1 response**: Queue supports single response per request

### Required Changes
1. **Async callback**: Push chunks immediately to queue
2. **Multi-response**: 1 request → N chunks + 1 final
3. **Non-blocking NANO**: Return streaming_id, poll chunks separately

## 4. Implementation Plan

### Phase 1: Callback Streaming (1-2 days)
```c
// Current: accumulate buffer
strcat(buffer, result->text);

// New: immediate streaming
queue_push(&response_queue, create_chunk(request_id, result->text));
```

### Phase 2: Queue Enhancement (2-3 days)
```c
typedef struct {
    uint32_t request_id;
    uint32_t chunk_sequence;
    bool is_streaming;
    bool is_final;
    char method[64];  // "stream_chunk" | "stream_final"
    // ... existing fields
} queue_item_t;
```

### Phase 3: NANO Streaming (3-4 days)
```c
// Non-blocking request
int nano_process_streaming_message() {
    io_push_streaming_request();
    return create_streaming_response(request_id);
}

// Separate chunk polling
int nano_poll_chunks(request_id, callback);
```

### Phase 4: Transport Integration (5-7 days)
- WebSocket streaming for real-time chunks
- HTTP chunked transfer encoding
- Handle lifecycle management

## 5. Architecture Benefits

**Universal Access:**
- Client gọi đầy đủ 15 RKLLM functions qua JSON-RPC
- Handle translation: `handle_id` → actual `LLMHandle` pointer
- Language agnostic: Python, JavaScript, any HTTP client

**Current Constraints:**
- Hardware: MAX_HANDLES=1, WORKER_COUNT=1 (RK3588 limitation)
- Future ready: Code designed for scaling when hardware improves

**Streaming Flow:**
```
RKLLM callback → immediate chunk → queue → NANO poll → client real-time
```

## 6. Conclusion

**Ready for Streaming:**
- ✅ RKLLM library có sẵn streaming (callback với RKLLM_RUN_NORMAL)
- ✅ Queue infrastructure có thể extend
- ✅ Handle management architecture đúng

**Implementation Required:**
- Modify callback để stream chunks thay vì accumulate
- Extend queue cho multi-response per request  
- Add non-blocking NANO interface

**Total Effort: ~2 weeks**

**Key Insight:** RKLLM đã hỗ trợ streaming sẵn qua callback - chúng ta chỉ cần implement đúng cách!
