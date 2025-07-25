# Complete RKNN API Implementation Plan

**Status**: High Priority - MatMul API Required for LLM Acceleration  
**Current Coverage**: 82% Core API (23/28 functions) + 0% MatMul API

## Phase 1: Complete Core API (1-2 days)

### Missing Core Function
```bash
src/rknn/call_rknn_create_mem_from_mb_blk/
├── call_rknn_create_mem_from_mb_blk.c
├── call_rknn_create_mem_from_mb_blk.h
└── CMakeLists.txt
```

**Function**: `rknn_tensor_mem* rknn_create_mem_from_mb_blk(rknn_context ctx, void *mb_blk, int32_t offset)`
- **Use Case**: Media buffer block integration
- **Priority**: Medium (platform-specific)

## Phase 2: RKNN MatMul API (3-4 days) - HIGH PRIORITY

### Directory Structure
```bash
src/rknn/matmul/
├── call_rknn_matmul_create/
├── call_rknn_matmul_destroy/
├── call_rknn_matmul_set_io_mem/
├── call_rknn_matmul_set_core_mask/
├── call_rknn_matmul_run/
├── call_rknn_matmul_set_quant_params/
├── call_rknn_matmul_get_quant_params/
├── call_rknn_matmul_create_dynamic_shape/
├── call_rknn_matmul_set_dynamic_shape/
└── call_rknn_B_normal_layout_to_native_layout/
```

### Implementation Priority Order

**High Priority (Essential)**:
1. `call_rknn_matmul_create/` - Create matrix multiplication context
2. `call_rknn_matmul_destroy/` - Destroy matmul context  
3. `call_rknn_matmul_set_io_mem/` - Set input/output memory
4. `call_rknn_matmul_run/` - Execute matrix multiplication
5. `call_rknn_matmul_set_core_mask/` - Set NPU core mask

**Medium Priority (Advanced)**:
6. `call_rknn_matmul_create_dynamic_shape/` - Dynamic shapes
7. `call_rknn_matmul_set_dynamic_shape/` - Set dynamic shape
8. `call_rknn_matmul_set_quant_params/` - Set quantization
9. `call_rknn_matmul_get_quant_params/` - Get quantization
10. `call_rknn_B_normal_layout_to_native_layout/` - Convert B matrix layout

### JSON-RPC Methods
```json
// Create MatMul Context
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "rknn.matmul.create",
  "params": {
    "M": 1024, "K": 4096, "N": 1024,
    "type": "RKNN_FLOAT16_MM_FLOAT16_TO_FLOAT32",
    "B_layout": 0, "AC_layout": 0
  }
}

// Run MatMul
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "rknn.matmul.run",
  "params": { "context_id": 0 }
}
```

### Global State Management
```c
// Add to existing RKNN state
extern rknn_matmul_ctx global_rknn_matmul_contexts[MAX_MATMUL_CONTEXTS];
extern int global_rknn_matmul_count;
```

## Implementation Coverage Analysis

| Category | Implemented | Total | Coverage |
|----------|-------------|-------|----------|
| **Core RKNN API** | 23 | 28 | 82% |
| Context Management | 4 | 4 | 100% |
| I/O Operations | 5 | 5 | 100% |
| Memory Management | 8 | 9 | 89% |
| **MatMul API** | 0 | 10 | 0% |
| **OVERALL** | **23** | **38** | **61%** |

## Success Metrics

### Phase 1 (Core API Complete)
- ✅ 100% core RKNN API coverage (28/28 functions)
- ✅ All core functions accessible via JSON-RPC
- ✅ No memory leaks or crashes

### Phase 2 (MatMul Implementation)  
- ✅ Hardware-accelerated matrix operations for LLMs
- ✅ Support for common transformer matrix sizes
- ✅ All MatMul functions implemented (10/10)
- ✅ Significant performance improvement for matrix ops

## Build System Updates
```cmake
# MatMul API symbols already available in librknnrt.so
# Add MatMul source directories to CMakeLists.txt
add_subdirectory(src/rknn/matmul/call_rknn_matmul_create)
add_subdirectory(src/rknn/matmul/call_rknn_matmul_destroy)
# ... additional MatMul directories
```

## Resource Requirements
- **Phase 1**: 1-2 days (1 missing function)
- **Phase 2**: 3-4 days (10 MatMul functions)
- **Testing**: 1 day per phase
- **Total**: ~6-8 days for 100% RKNN API coverage

**Next Action**: Prioritize MatMul API implementation for hardware-accelerated matrix operations critical to LLM performance.