# NANO + IO System - Production Plan

## System Overview

```
Clients → [UDP|TCP|HTTP|WS|STDIO] → NANO → IO → RKLLM
```

**Status**: PRODUCT IS IN HOT DEVELOPMENT AND IS NOT READY!
**Status**: PRODUCT IS IN HOT DEVELOPMENT AND IS NOT READY!
**Status**: PRODUCT IS IN HOT DEVELOPMENT AND IS NOT READY!

## 1. IO Layer (Core Engine)

### Architecture
- **Queue-based**: Request/Response queues with worker pool
- **Handle mapping**: LLMHandle* ↔ unique_id translation  
- **Thread-safe**: Atomic operations, 5 worker threads
- **Memory managed**: Pool-based allocation, NPU memory detection

### Function Prefix Handling
- **Nano requests**: Send function names WITHOUT "rkllm_" prefix
- **IO mapping**: Automatically prepends "rkllm_" to call correct RKLLM functions
- **Example**: Nano sends "init" → IO calls "rkllm_init"

### Public Interface
```c
// io.h
int io_init(void);
int io_push_request(const char* json_request);
int io_pop_response(char* json_response, size_t max_len);
void io_shutdown(void);
```

### Handle Management
- **Pool capacity**: 8 concurrent handles maximum
- **ID mapping**: uint32_t → LLMHandle* translation
- **NPU memory**: Dynamic detection (8GB/16GB based on system RAM)
- **Model support**: QwenVL (7.7GB), LoRA (4.2GB)

## 2. Nano Layer (MCP Gateway)

### Protocol Support
- **JSON-RPC 2.0**: MCP specification compliant
- **Transports**: UDP, TCP, HTTP, WebSocket, STDIO
- **Stateless**: Forward-only design, no state retention
- **Error handling**: MCP compliant responses

### Transport Interface
```c
// transport.h
typedef struct {
    int (*init)(int port);
    int (*send)(const char* data);
    int (*recv)(char* buffer, size_t len);
    void (*close)(void);
} transport_ops_t;
```

## 3. System Information & Validation

### Model Validation
- **Compatibility**: Resource-based validation only
- **Version check**: ✅ **DEPRECATED** - Simplified for reliability
- **Memory check**: NPU requirements vs available memory
- **File validation**: Existence and accessibility

### System Detection
```c
// RK3588 Platform
- CPU cores: 8
- NPU cores: 3  
- NPU memory: 8GB (standard) / 16GB (high-end systems)
- RAM detection: Dynamic based on total system memory
```

## 4. Model Configuration

### Supported Models
```json
{
  "qwenvl": {
    "file": "models/qwenvl/model.rkllm",
    "size": "7.7GB", 
    "npu_required": "11.7GB",
    "status": "✅ Working with 16GB NPU"
  },
  "lora": {
    "file": "models/lora/model.rkllm", 
    "size": "4.2GB",
    "npu_required": "6.3GB",
    "status": "✅ Working with 8GB/16GB NPU"
  }
}
```

## 5. C23 Implementation

### Modern Features
- **constexpr**: Compile-time constants
- **nullptr**: Type-safe null pointer
- **[[nodiscard]]**: Return value validation
- **auto**: Type deduction
- **_Generic**: Type-safe macros

### Header Organization
- **Consolidated**: `src/common/core.h` unified header
- **No legacy**: Pure C23, no C99/C11 compatibility
- **Clean includes**: Minimal, organized structure

## 6. Test Suite Structure

### Test Coverage
```
✅ Common Utilities    - JSON parsing, memory utils
✅ IO Layer           - Queue operations, worker pool
✅ Nano Validation    - Model compatibility (simplified)
✅ Nano System        - Resource detection, memory management
✅ Integration QwenVL - 7.7GB model loading & inference
✅ Integration LoRA   - 4.2GB model loading & inference
```

### Test Categories
- **Unit tests**: Component isolation testing
- **Integration tests**: Real model validation
- **Resource tests**: Memory and NPU validation
- **System tests**: Platform detection

## 7. Build System

### Make Targets
```bash
make all          # Build nano, io, test
make nano         # Build nano executable  
make io           # Build io executable
make test         # Build and run tests
make clean        # Clean artifacts
```

### Build Configuration
- **Compiler**: clang with C23 standard
- **Libraries**: librkllmrt.so (RKLLM runtime)
- **Threading**: pthread support
- **Optimization**: Production-ready flags

## 8. Current File Structure

```
src/
├── io/                    # IO core engine
│   ├── core/             # Queue, worker pool, operations
│   └── mapping/          # Handle pool, RKLLM proxy
├── nano/                 # MCP gateway
│   ├── system/           # System info, resource management
│   ├── validation/       # Model compatibility (simplified)
│   ├── transport/        # Protocol implementations
│   └── core/             # Nano main logic
├── common/               # Shared utilities (C23)
└── libs/rkllm/          # RKLLM library & headers

tests/                    # Test suite
├── common/              # Utility tests
├── io/                  # IO layer tests  
├── nano/                # Nano layer tests
└── integration/         # Model integration tests

config/models.json       # Model configurations
models/                  # Model files
├── qwenvl/model.rkllm  # QwenVL model (7.7GB)
└── lora/model.rkllm    # LoRA model (4.2GB)
```

## 10. Deployment

### Runtime Usage
```bash
# Run complete test suite
./test

# Individual components (when available)
./nano --config config/models.json
./io --standalone-mode
```

### System Requirements
- **Platform**: RK3588 or compatible
- **RAM**: 16GB+ recommended for QwenVL
- **NPU**: 3 cores minimum
- **Storage**: Models directory with .rkllm files