# Issue: Add RKNN Support for Vision Models (Primitive API Only)

**Date**: 2025-07-24  
**Priority**: High  
**Type**: Feature Implementation

## Problem Statement

Our nano server currently only supports `.rkllm` models through primitive RKLLM API calls. To enable clients to build multimodal applications (like Qwen2-VL), we need to add primitive RKNN runtime support for vision models (`.rknn` files).

**Server Scope**: Provide primitive 1:1 API mappings to RKNN functions only
**Client Scope**: High-level multimodal orchestration, image preprocessing, etc.

Required primitive support:
- **RKNN Runtime**: Primitive access to `.rknn` vision models  
- **RKLLM Runtime**: Existing primitive access to `.rkllm` language models

## Current State

- ✅ RKLLM runtime integrated (`librkllmrt.so`) 
- ✅ Primitive RKLLM API methods available
- ❌ RKNN runtime not integrated (`librknnrt.so`)
- ❌ Primitive RKNN API methods missing

## Proposed Solution

### 1. Architecture Design

```
nano/
├── external/
│   ├── rknn-llm/        (existing)
│   └── rknn-toolkit2/   (new)
│       └── rknpu2/
│           └── runtime/
│               └── Linux/
│                   └── librknn_api/
│                       ├── aarch64/
│                       │   └── librknnrt.so
│                       └── include/
│                           └── rknn_api.h
├── src/
│   ├── rkllm/           (existing - primitive RKLLM wrappers)
│   └── rknn/            (new - primitive RKNN wrappers)
│       ├── call_rknn_init/
│       ├── call_rknn_query/  
│       ├── call_rknn_run/
│       └── call_rknn_destroy/
└── tests/
    ├── images/          (existing test images)
    └── multimodal/      (new test suite)
```

### 2. Implementation Steps

#### Phase 1: RKNN Runtime Integration  
1. **Update CMakeLists.txt**:
   - Add RKNN toolkit download logic (similar to RKLLM)
   - Link both `librknnrt.so` and `librkllmrt.so`
   - Include RKNN headers

2. **Create primitive RKNN wrapper functions** (1:1 API mapping):
   - `call_rknn_init()` - Direct wrapper for `rknn_init()`
   - `call_rknn_query()` - Direct wrapper for `rknn_query()`  
   - `call_rknn_run()` - Direct wrapper for `rknn_run()`
   - `call_rknn_destroy()` - Direct wrapper for `rknn_destroy()`

#### Phase 2: JSON-RPC Integration
3. **Add primitive JSON-RPC methods**:
   - `rknn.init` - Initialize vision model
   - `rknn.query` - Query model information  
   - `rknn.run` - Run vision inference
   - `rknn.destroy` - Cleanup vision model

4. **No server-side orchestration** - clients handle:
   - Model loading coordination
   - Image preprocessing  
   - Cross-attention parameter passing
   - Multimodal inference logic

#### Phase 3: Testing & Validation  
5. **Test primitive functions**:
   - Verify each RKNN API wrapper works independently
   - Test with `.rknn` model files
   - Ensure clients can build multimodal workflows

### 3. Primitive API Design (1:1 RKNN Mapping)

#### Vision Model Initialization
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "rknn.init",
  "params": {
    "model_path": "/path/to/vision.rknn",
    "core_mask": 1
  }
}
```

#### Model Information Query
```json
{
  "jsonrpc": "2.0", 
  "id": 2,
  "method": "rknn.query",
  "params": {
    "query_type": "sdk_version"  // or "input_attr", "output_attr"
  }
}
```

#### Vision Inference (Primitive)
```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "rknn.run", 
  "params": {
    "input_data": "base64_encoded_tensor"  // Client preprocesses image
  }
}
```

**Note**: No high-level `multimodal.process` - clients orchestrate RKNN + RKLLM calls

### 4. Server Scope & Client Responsibilities

#### Server Provides (Primitive Only):
- Direct RKNN API access: init, query, run, destroy
- Direct RKLLM API access: existing methods
- Basic tensor I/O handling
- Memory management per model

#### Client Handles (High-Level Logic):
- Image preprocessing (resize, normalize, etc.)
- Vision-language model coordination
- Cross-attention parameter management  
- Multimodal inference workflows
- Application-specific optimizations

### 5. Build System Updates

```cmake
# Add RKNN toolkit download
set(RKNN_REPO_URL "https://github.com/airockchip/rknn-toolkit2.git")
set(RKNN_DIR "${CMAKE_SOURCE_DIR}/external/rknn-toolkit2")
set(RKNN_RUNTIME_DIR "${RKNN_DIR}/rknpu2/runtime/Linux/librknn_api")
set(RKNN_LIB_PATH "${RKNN_RUNTIME_DIR}/aarch64/librknnrt.so")

# Link both libraries
target_link_libraries(server
    ${CMAKE_THREAD_LIBS_INIT}
    rkllmrt
    rknnrt  # New addition
    ${JSON_C_LIBRARIES}
)
```

## Success Criteria

1. ✅ RKNN runtime successfully integrated
2. ✅ Primitive RKNN methods available via JSON-RPC
3. ✅ Vision models (.rknn) can be loaded and run independently  
4. ✅ Each RKNN function works as direct API wrapper
5. ✅ Clients can build multimodal applications using primitives

## Testing Plan

1. **Unit Tests**:
   - Test each primitive RKNN wrapper function independently
   - Verify JSON-RPC method dispatching  

2. **API Tests**:
   - Load `.rknn` models via `rknn.init`
   - Query model info via `rknn.query`
   - Run basic inference via `rknn.run`

3. **Client Integration**:
   - Verify clients can orchestrate RKNN + RKLLM calls
   - Test primitive building blocks for multimodal apps

## Risk Mitigation

- **API Compatibility**: Ensure 1:1 mapping with RKNN library functions
- **Memory Management**: Each model handles its own NPU memory allocation
- **Error Handling**: Return proper JSON-RPC errors for failed RKNN calls

## Timeline Estimate

- Phase 1 (RKNN Integration): 1-2 days
- Phase 2 (JSON-RPC Methods): 1 day  
- Phase 3 (Testing): 1 day
- **Total**: ~3-4 days (simplified scope)

## References

- [RKNN Toolkit2 Repository](https://github.com/airockchip/rknn-toolkit2)
- [RKNN-LLM Repository](https://github.com/airockchip/rknn-llm)
- [Qwen2-VL Implementation Example](https://github.com/thanhtantran/Qwen2-VL-Streamlit-App-with-RKLLM/)