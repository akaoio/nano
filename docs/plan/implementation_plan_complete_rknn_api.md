# Implementation Plan: Complete RKNN API Coverage

**Date**: 2025-07-24 16:00  
**Priority**: High  
**Scope**: Implement Missing RKNN Functions for Full API Coverage

## Overview

Based on the gap analysis, we need to implement the remaining RKNN API functions to achieve 100% coverage. This plan focuses on the most critical missing features: MatMul API and remaining core functions.

## Implementation Phases

### Phase 1: Complete Core RKNN API (1-2 days)

#### 1.1 Missing Core Function Implementation
- **Target**: 100% core RKNN API coverage (currently 82%)

##### Memory Management Extension
```bash
src/rknn/call_rknn_create_mem_from_mb_blk/
├── call_rknn_create_mem_from_mb_blk.c
├── call_rknn_create_mem_from_mb_blk.h
└── CMakeLists.txt
```

**Function**: `rknn_tensor_mem* rknn_create_mem_from_mb_blk(rknn_context ctx, void *mb_blk, int32_t offset)`
- **Use case**: Media buffer block integration
- **Priority**: Medium (platform-specific)

#### 1.2 JSON-RPC Integration
- Add `rknn.create_mem_from_mb_blk` method
- Update method dispatcher in `handle_request.c`

### Phase 2: RKNN MatMul API Implementation (3-4 days)

#### 2.1 Directory Structure Creation
```bash
src/rknn/matmul/
├── call_rknn_matmul_create/
├── call_rknn_matmul_create_dynamic_shape/
├── call_rknn_matmul_destroy/
├── call_rknn_matmul_set_io_mem/
├── call_rknn_matmul_set_core_mask/
├── call_rknn_matmul_set_quant_params/
├── call_rknn_matmul_get_quant_params/
├── call_rknn_matmul_set_dynamic_shape/  
├── call_rknn_matmul_run/
├── call_rknn_B_normal_layout_to_native_layout/
└── get_rknn_matmul_constants/
```

#### 2.2 Priority Implementation Order

##### High Priority (Critical for LLM acceleration)
1. **Context Management**
   - `call_rknn_matmul_create/` - Create matrix multiplication context
   - `call_rknn_matmul_destroy/` - Destroy matmul context

2. **Basic Operations**  
   - `call_rknn_matmul_set_io_mem/` - Set input/output memory
   - `call_rknn_matmul_run/` - Execute matrix multiplication

3. **Core Configuration**
   - `call_rknn_matmul_set_core_mask/` - Set NPU core mask

##### Medium Priority (Advanced features)
4. **Dynamic Shape Support**
   - `call_rknn_matmul_create_dynamic_shape/` - Create with dynamic shapes
   - `call_rknn_matmul_set_dynamic_shape/` - Set dynamic shape

5. **Quantization Support**
   - `call_rknn_matmul_set_quant_params/` - Set quantization parameters
   - `call_rknn_matmul_get_quant_params/` - Get quantization parameters

6. **Layout Utilities**
   - `call_rknn_B_normal_layout_to_native_layout/` - Convert B matrix layout

#### 2.3 Global State Management
```c
// Add to existing RKNN global state
extern rknn_matmul_ctx global_rknn_matmul_contexts[MAX_MATMUL_CONTEXTS];
extern int global_rknn_matmul_count;
extern int global_rknn_matmul_initialized;
```

#### 2.4 JSON-RPC Methods
- `rknn.matmul.create` - Create matmul context
- `rknn.matmul.create_dynamic_shape` - Create with dynamic shapes  
- `rknn.matmul.destroy` - Destroy matmul context
- `rknn.matmul.set_io_mem` - Set I/O memory
- `rknn.matmul.set_core_mask` - Set core mask
- `rknn.matmul.set_quant_params` - Set quantization  
- `rknn.matmul.get_quant_params` - Get quantization
- `rknn.matmul.set_dynamic_shape` - Set dynamic shape
- `rknn.matmul.run` - Execute matrix multiplication
- `rknn.matmul.convert_layout` - Convert B matrix layout

### Phase 3: RKNN Custom Op API (Optional - 2-3 days)

#### 3.1 Directory Structure  
```bash
src/rknn/custom_op/
├── call_rknn_register_custom_ops/
├── call_rknn_custom_op_get_op_attr/
└── get_rknn_custom_op_constants/
```

#### 3.2 Implementation Priority
- **Priority**: Low (advanced extensibility feature)
- **Use case**: Custom operator implementations
- **Decision**: Implement only if specific custom operators needed

## Technical Implementation Details

### MatMul Context Management Pattern
```c
// Following existing RKNN pattern
typedef struct {
    rknn_matmul_ctx ctx;
    int initialized;
    char name[256];
} matmul_context_info;

static matmul_context_info matmul_contexts[MAX_MATMUL_CONTEXTS];
static int matmul_context_count = 0;
```

### JSON-RPC Method Examples

#### Create MatMul Context
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "rknn.matmul.create",
  "params": {
    "M": 1024,
    "K": 4096, 
    "N": 1024,
    "type": "RKNN_FLOAT16_MM_FLOAT16_TO_FLOAT32",
    "B_layout": 0,
    "AC_layout": 0
  }
}
```

#### Run MatMul
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "rknn.matmul.run",
  "params": {
    "context_id": 0
  }
}
```

### Error Handling Strategy
- Follow existing RKNN error code patterns
- Map RKNN MatMul error codes to JSON-RPC errors
- Provide detailed error messages for debugging

## Build System Updates

### CMakeLists.txt Modifications
```cmake
# Add MatMul API symbols (already available in librknnrt.so)
# No additional linking required - symbols included in existing library

# Add MatMul source directories
add_subdirectory(src/rknn/matmul/call_rknn_matmul_create)
add_subdirectory(src/rknn/matmul/call_rknn_matmul_destroy)
# ... additional MatMul directories
```

## Testing Strategy

### Phase 1 Testing (Core API)
1. **Unit Tests**: Test `rknn_create_mem_from_mb_blk` wrapper
2. **Integration Tests**: Verify JSON-RPC method dispatching

### Phase 2 Testing (MatMul API)
1. **Unit Tests**: Test each MatMul wrapper function
2. **Performance Tests**: Benchmark MatMul vs standard operations
3. **Integration Tests**: End-to-end MatMul workflow testing
4. **Memory Tests**: Verify proper memory management

### Test Models Required
- Simple matrix multiplication test cases
- LLM-style transformer layer matrices
- Various quantization types (INT8, FP16, etc.)

## Success Metrics

### Phase 1 Success Criteria
- ✅ 100% core RKNN API coverage (28/28 functions)
- ✅ All core functions work via JSON-RPC
- ✅ No memory leaks or crashes

### Phase 2 Success Criteria  
- ✅ All MatMul API functions implemented (14/14 functions)
- ✅ MatMul JSON-RPC methods functional
- ✅ Significant performance improvement for matrix operations
- ✅ Support for common LLM matrix sizes and types

## Risk Mitigation

### Technical Risks
1. **MatMul Complexity**: Start with basic create/destroy/run functions
2. **Memory Management**: Follow existing RKNN memory patterns
3. **Performance Issues**: Profile MatMul operations early

### Timeline Risks
1. **Scope Creep**: Focus on essential MatMul functions first
2. **Testing Overhead**: Prioritize critical path testing

## Resource Requirements

### Development Time
- **Phase 1**: 1-2 days (low complexity)
- **Phase 2**: 3-4 days (medium-high complexity)
- **Phase 3**: 2-3 days (optional, low priority)

### Testing Time
- **Unit Testing**: 1 day per phase
- **Integration Testing**: 1 day total
- **Performance Testing**: 1 day (Phase 2 only)

## Next Steps

1. **Immediate**: Begin Phase 1 implementation
2. **Week 1**: Complete core API coverage
3. **Week 2**: Begin MatMul API implementation  
4. **Week 3**: Complete MatMul API and testing
5. **Optional**: Phase 3 Custom Op API if needed

This implementation will provide **100% RKNN API coverage** and unlock **hardware-accelerated matrix operations** critical for high-performance LLM inference.