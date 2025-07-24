# Implementation Plan: RKNN Primitive API Support

## Overview

This document outlines the implementation plan for adding primitive RKNN API support to the nano server. The server will provide only 1:1 mappings to RKNN library functions, with high-level multimodal logic handled by clients.

## Implementation Phases

### Phase 1: RKNN Runtime Integration (Days 1-2)

#### 1.1 Update Build System
- [ ] Modify CMakeLists.txt to download rknn-toolkit2
- [ ] Add librknnrt.so linking alongside librkllmrt.so
- [ ] Include RKNN API headers (rknn_api.h, rknn_matmul_api.h)
- [ ] Update build script generation

#### 1.2 Create RKNN Directory Structure (Primitive Only)
```bash
src/rknn/
├── call_rknn_init/        # Direct rknn_init() wrapper
├── call_rknn_query/       # Direct rknn_query() wrapper  
├── call_rknn_run/         # Direct rknn_run() wrapper
└── call_rknn_destroy/     # Direct rknn_destroy() wrapper
```

#### 1.3 Implement Primitive RKNN Wrappers (1:1 API Mapping)
Following the one-function-per-file rule:
- `call_rknn_init()` - Direct wrapper for `rknn_init()`
- `call_rknn_query()` - Direct wrapper for `rknn_query()`  
- `call_rknn_run()` - Direct wrapper for `rknn_run()`  
- `call_rknn_destroy()` - Direct wrapper for `rknn_destroy()`

### Phase 2: JSON-RPC Interface (Day 3)

#### 2.1 Add Primitive RKNN Methods
- [x] `rknn.init` - Direct access to rknn_init()
- [x] `rknn.query` - Direct access to rknn_query()
- [x] `rknn.run` - Direct access to rknn_run()
- [x] `rknn.destroy` - Direct access to rknn_destroy()

#### 2.2 No Server-Side Orchestration
**Explicitly NOT implemented** (client responsibility):
- ~~`multimodal.init`~~ - Clients coordinate model loading
- ~~`multimodal.process`~~ - Clients orchestrate RKNN + RKLLM calls
- ~~Image preprocessing~~ - Clients handle image preprocessing
- ~~Cross-attention handling~~ - Clients manage model interactions

### Phase 3: Testing (Day 4)

#### 3.1 Unit Testing
- [x] Test each primitive RKNN wrapper function
- [x] Verify JSON-RPC method dispatching
- [x] Test basic model loading and querying

#### 3.2 Client Integration Testing  
- [ ] Verify clients can load .rknn models via `rknn.init`
- [ ] Test that clients can query model info via `rknn.query` 
- [ ] Confirm clients can run inference via `rknn.run`
- [ ] Validate that clients can orchestrate RKNN + RKLLM calls

**Note**: Server testing focuses on primitive API correctness only. Multimodal application testing is the client's responsibility.

## Technical Details

### Primitive API Examples (Server Scope)

#### Initialize Vision Model
```json
// Request
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "rknn.init",
  "params": {
    "model_path": "/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn",
    "core_mask": 1
  }
}

// Response  
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "success": true,
    "message": "RKNN model initialized successfully"
  }
}
```

#### Query Model Information
```json
// Request
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "rknn.query",
  "params": {
    "query_type": "input_attr"
  }
}

// Response
{
  "jsonrpc": "2.0", 
  "id": 2,
  "result": {
    "input_attrs": [
      {
        "index": 0,
        "dims": [1, 3, 448, 448],
        "size": 2408448,
        "type": "RKNN_TENSOR_UINT8"
      }
    ]
  }
}
```

### Client Workflow Example (Not Server Scope)

This shows how clients would use the primitive server APIs:

```bash
# 1. Client initializes vision model
curl -X POST -d '{"jsonrpc":"2.0","id":1,"method":"rknn.init","params":{"model_path":"/models/vision.rknn","core_mask":1}}'

# 2. Client initializes language model  
curl -X POST -d '{"jsonrpc":"2.0","id":2,"method":"rkllm.init","params":{"model_path":"/models/language.rkllm"}}'

# 3. Client preprocesses image (client-side logic)
# 4. Client calls rknn.run with preprocessed tensor
# 5. Client processes RKNN output (client-side logic)  
# 6. Client calls rkllm.run with vision embeddings
# 7. Client handles streaming text output
```

**Note**: All image preprocessing, model coordination, and multimodal logic happens on the client side. The server only provides primitive RKNN and RKLLM API access.

## Server Testing Strategy (Primitive Focus)

### Unit Tests
1. Test each primitive RKNN wrapper function independently
2. Test JSON-RPC method dispatching 
3. Test basic model loading/unloading

### Client Integration Tests  
1. Verify clients can use primitive APIs to build multimodal workflows
2. Test that primitive APIs provide sufficient building blocks
3. Validate error handling for malformed requests

**Note**: Client-side multimodal testing (image descriptions, etc.) is the client's responsibility.

## Deliverables

1. **Updated CMakeLists.txt** with RKNN support ✅
2. **Primitive RKNN wrapper functions** in `src/rknn/` ✅  
3. **Extended JSON-RPC API** with primitive RKNN methods ✅
4. **Unit test suite** for primitive functions ✅
5. **Updated documentation** reflecting primitive-only scope ✅

## Success Metrics

- ✅ Successfully load and run `.rknn` vision models via primitive API
- ✅ Each RKNN wrapper function works as direct 1:1 mapping
- ✅ JSON-RPC methods dispatch correctly  
- ✅ Clients can orchestrate multimodal workflows using primitives
- ✅ Build system supports both RKNN and RKLLM libraries ✅
- ✅ No memory leaks or crashes

## Risk Mitigation

1. **Library Compatibility**: Test on target hardware early
2. **Memory Constraints**: Implement proper cleanup and monitoring
3. **Performance Issues**: Profile and optimize critical paths
4. **API Changes**: Design flexible interface for future updates

## Next Steps

1. Begin Phase 1 with CMakeLists.txt updates
2. Set up RKNN directory structure
3. Implement first RKNN wrapper function
4. Test basic RKNN functionality
5. Proceed with remaining phases

This implementation will enable the nano server to support state-of-the-art multimodal AI models, significantly expanding its capabilities beyond text-only inference.