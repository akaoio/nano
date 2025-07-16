# Kế hoạch triển khai dự án nano + io cho RKLLM

## Tổng quan kiến trúc

```
┌─────────────────────────────────────────────────────────────┐
│                        nano (MCP Gateway)                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │   UDP       │  │   TCP       │  │   HTTP      │          │
│  │   WS        │  │   STDIO     │  │   ...       │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
│                           │                                 │
│                    ┌─────────────┐                          │
│                    │   MCP       │                          │
│                    │   Protocol  │                          │
│                    └─────────────┘                          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        io (Core Engine)                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                  Request Queue                      │    │
│  │  [nano] → [handle_id:123, op:"run", params:...]     │    │
│  └─────────────────────────────────────────────────────┘    │
│                              │                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              Worker Thread Pool                     │    │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐            │    │
│  │  │H1   │ │H2   │ │H3   │ │H4   │ │H5   │            │    │
│  │  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘            │    │
│  └─────────────────────────────────────────────────────┘    │
│                              │                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                 Response Queue                      │    │
│  │  [handle_id:123, status:0, result:...] → [nano]     │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        RKLLM Library                        │
│                    (librkllmrt.so)                          │
└─────────────────────────────────────────────────────────────┘
```

## 1. IO Layer - Trái tim hệ thống

### 1.1 Thiết kế tối giản
- **Chức năng**: Queue system + handle mapping + RKLLM operations
- **Không logic**: Chỉ ánh xạ trực tiếp RKLLM functions
- **Thread-safe**: Mỗi handle có worker thread riêng
- **Memory efficient**: Pool-based allocation, không dynamic allocation

### 1.2 Cấu trúc file
```
src/io/
├── io.h              // Public interface (4 hàm duy nhất)
├── io.c              // Core implementation
├── handle_pool.h     // Handle management
├── handle_pool.c     // Handle pool implementation
├── queue.h           // Lock-free queue
├── queue.c           // Queue implementation
├── operations.h      // RKLLM operation mapping
└── operations.c      // Auto-generated operations
```

### 1.3 Public Interface (chỉ 4 hàm)
```c
// io.h
int io_init(void);
int io_push_request(const char* json_request);
int io_pop_response(char* json_response, size_t max_len);
void io_shutdown(void);
```

### 1.4 Handle Management
- **Pool tĩnh**: Tối đa 8 handles đồng thời
- **ID mapping**: uint32_t → LLMHandle*
- **Memory tracking**: Theo dõi memory usage mỗi handle
- **Auto-cleanup**: Tự động destroy khi memory pressure

### 1.5 Queue System
- **Dual queue**: Request queue + Response queue
- **Lock-free**: Sử dụng atomic operations
- **Batch processing**: Hỗ trợ batch requests
- **Timeout handling**: Request timeout sau 30s

## 2. Nano Layer - MCP Gateway

### 2.1 Thiết kế
- **Protocol**: JSON-RPC 2.0 + MCP specification
- **Transports**: UDP, TCP, HTTP, WebSocket, STDIO
- **Stateless**: Không lưu trạng thái, chỉ forward requests
- **Error handling**: MCP compliant error responses

### 2.2 Cấu trúc file
```
src/nano/
├── nano.h              // Public interface
├── nano.c              // Core nano logic
├── mcp/
│   ├── mcp.h           // MCP protocol definitions
│   ├── mcp.c           // MCP implementation
│   └── json_rpc.h      // JSON-RPC parser
├── transports/
│   ├── transport.h     // Transport interface
│   ├── stdio.c         // STDIO transport
│   ├── tcp.c           // TCP transport
│   ├── udp.c           // UDP transport
│   ├── http.c          // HTTP transport
│   └── ws.c            // WebSocket transport
└── main.c              // Entry point
```

### 2.3 Transport Interface
```c
// transport.h
typedef struct {
    int (*init)(int port);
    int (*send)(const char* data);
    int (*recv)(char* buffer, size_t len);
    void (*close)(void);
} transport_ops_t;
```

## 3. Build System Modular

### 3.1 Makefile structure
```makefile
# Build từng component riêng
libio.so: src/io/*.c
	gcc -std=c23 -shared -fPIC -o $@ $^ -lrkllmrt

libnano.so: src/nano/*.c src/nano/transports/*.c
	gcc -std=c23 -shared -fPIC -o $@ $^ -ljson-c

nano: src/main.c libio.so libnano.so
	gcc -std=c23 -o $@ $< -L. -lio -lnano

# Test riêng từng component
test-io: tests/io/test_io.c libio.so
	gcc -std=c23 -o $@ $< -L. -lio

test-nano: tests/test_nano.c libnano.so
	gcc -std=c23 -o $@ $< -L. -lnano
```

### 3.2 Test structure
```
tests/
├── test.c                 # Main test entry point
├── io/                    # IO layer tests
│   ├── test_io.c          # Main IO test runner
│   ├── test_io.h          # IO test header
│   ├── test_io_init.c     # IO initialization tests
│   ├── test_model_files.c # Model file tests
│   ├── test_qwenvl_loading.c # QwenVL loading tests
│   ├── test_lora_loading.c   # LoRA loading tests
│   ├── test_inference.c      # Inference tests
│   ├── test_error_cases.c    # Error handling tests
│   └── test_cleanup.c        # Cleanup tests
├── models/
│   ├── test_qwenvl.json   # Test config cho qwenvl
│   └── test_lora.json     # Test config cho lora
└── fixtures/
    ├── sample_requests.json
    └── expected_responses.json
```

## 4. C23 Features sử dụng

### 4.1 Modern C features
- `constexpr` cho constants
- `auto` type inference
- Designated initializers
- Attributes: `[[nodiscard]]`, `[[maybe_unused]]`
- `nullptr` thay vì NULL
- `_Generic` cho type-safe macros

### 4.2 Code generation
- **Auto-mapping**: Tự động generate RKLLM operation mapping
- **Macro magic**: Tự động tạo wrapper functions
- **Compile-time**: Compile-time string hashing cho operation lookup

## 5. Multi-model Support

### 5.1 Configuration
```json
// config/models.json
{
  "models": {
    "qwenvl": {
      "path": "models/qwenvl/model.rkllm",
      "max_instances": 2,
      "memory_limit_mb": 8000
    },
    "lora": {
      "path": "models/lora/model.rkllm",
      "max_instances": 1,
      "memory_limit_mb": 4000
    }
  }
}
```

### 5.2 Resource management
- **Memory tracking**: Theo dõi memory usage mỗi handle
- **Auto-scaling**: Tự động scale instances dựa trên load
- **Graceful degradation**: Giảm instances khi memory pressure

## 6. Implementation Timeline

### Phase 1: Core IO (2 ngày)
- [ ] Day 1: Queue system + Handle pool
- [ ] Day 2: Operation mapping + Unit tests

### Phase 2: Nano MCP (2 ngày)
- [ ] Day 3: MCP protocol + JSON-RPC parser
- [ ] Day 4: Transport implementations + Unit tests

### Phase 3: Integration (1 ngày)
- [ ] Day 5: End-to-end testing + Performance tuning

### Phase 4: Optimization (1 ngày)
- [ ] Day 6: Memory optimization + Error handling
- [ ] Day 7: Documentation + Final testing

## 7. Testing Strategy

### 7.1 Unit tests
- **IO tests**: Queue operations, handle management
- **Nano tests**: MCP protocol compliance, transport tests
- **Integration tests**: Real model tests với qwenvl và lora

### 7.2 Performance tests
- **Throughput**: Requests per second
- **Latency**: Response time distribution
- **Memory**: Memory usage patterns
- **Scalability**: Multi-handle performance

## 8. Error Handling

### 8.1 IO errors
- **Handle not found**: Return error với handle_id
- **Memory limit**: Return RESOURCE_EXHAUSTED
- **Queue full**: Return QUEUE_FULL với retry_after

### 8.2 Nano errors
- **Invalid JSON**: Return PARSE_ERROR
- **Invalid operation**: Return METHOD_NOT_FOUND
- **Transport errors**: Return CONNECTION_ERROR

## 9. Security Considerations

### 9.1 Input validation
- **JSON validation**: Validate tất cả JSON input
- **Path validation**: Validate model paths
- **Size limits**: Limit request/response sizes

### 9.2 Resource limits
- **Memory limits**: Per-handle memory limits
- **Time limits**: Request timeout
- **Rate limiting**: Per-client rate limits

## 10. Deployment

### 10.1 Build targets
```bash
make libio.so      # Build IO library
make libnano.so    # Build Nano library
make nano          # Build executable
make test          # Run all tests
make install       # Install to system
```

### 10.2 Runtime configuration
```bash
# Start nano với tất cả transports
./nano --config config/models.json --transports all

# Start nano với specific transports
./nano --transports tcp:8080,http:8081,stdio
```

## 11. Monitoring & Debugging

### 11.1 Metrics
- **Request count**: Per-operation counters
- **Latency**: P50, P95, P99 latencies
- **Memory usage**: Per-handle memory tracking
- **Error rates**: Per-error-type counters

### 11.2 Debug mode
```bash
# Debug mode với verbose logging
./nano --debug --log-level debug

# Performance profiling
./nano --profile --output perf.json
```

## 12. Code Organization

```
src/
├── io/                    # Core IO layer
├── nano/                  # MCP gateway
├── common/               # Shared utilities
├── config/               # Configuration files
└── tests/                # Test suite
```

## 13. Success Criteria

- [ ] IO layer < 500 LOC
- [ ] Nano layer < 300 LOC
- [ ] Zero memory leaks
- [ ] 100% test coverage cho core logic
- [ ] Support 8 concurrent handles
- [ ] < 1ms queue overhead
- [ ] MCP compliant
- [ ] All transports functional
- [ ] Real model tests pass
- [ ] Performance benchmarks documented
