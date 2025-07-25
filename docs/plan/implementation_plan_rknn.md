# Implementation Plan: RKNN Primitive API Support

## Overview
Add primitive RKNN API support to nano server for vision models. Server provides 1:1 mappings to RKNN library functions only.

## Implementation Phases

### Phase 1: RKNN Runtime Integration ✅
- [x] CMakeLists.txt updated with rknn-toolkit2 download
- [x] librknnrt.so linking alongside librkllmrt.so  
- [x] RKNN API headers included
- [x] Build system supports both libraries

### Phase 2: Primitive RKNN Wrappers ✅
Directory structure (1:1 API mapping):
```bash
src/rknn/
├── call_rknn_init/        # Direct rknn_init() wrapper
├── call_rknn_query/       # Direct rknn_query() wrapper  
├── call_rknn_run/         # Direct rknn_run() wrapper
└── call_rknn_destroy/     # Direct rknn_destroy() wrapper
```

### Phase 3: JSON-RPC Interface ✅
- [x] `rknn.init` - Direct access to rknn_init()
- [x] `rknn.query` - Direct access to rknn_query()
- [x] `rknn.run` - Direct access to rknn_run()
- [x] `rknn.destroy` - Direct access to rknn_destroy()

## API Examples

### Initialize Vision Model
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "rknn.init",
  "params": {
    "model_path": "/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn",
    "core_mask": 1
  }
}
```

### Query Model Information
```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "rknn.query",
  "params": {
    "query_type": "input_attr"
  }
}
```

## Client Workflow Example
```bash
# 1. Client initializes vision model
curl -X POST -d '{"jsonrpc":"2.0","id":1,"method":"rknn.init","params":{"model_path":"/models/vision.rknn","core_mask":1}}'

# 2. Client initializes language model  
curl -X POST -d '{"jsonrpc":"2.0","id":2,"method":"rkllm.init","params":{"model_path":"/models/language.rkllm"}}'

# 3. Client handles image preprocessing + coordination
# 4. Client calls rknn.run with preprocessed tensor
# 5. Client calls rkllm.run with vision embeddings
```

## Server Scope (What We Provide)
- Direct RKNN API access: init, query, run, destroy
- Direct RKLLM API access: existing methods
- Basic tensor I/O handling
- Memory management per model

## Client Responsibility (What We DON'T Provide)
- Image preprocessing (resize, normalize, etc.)
- Vision-language model coordination
- Cross-attention parameter management  
- Multimodal inference workflows

## Deliverables ✅
1. **Updated CMakeLists.txt** with RKNN support
2. **Primitive RKNN wrapper functions** in `src/rknn/`  
3. **Extended JSON-RPC API** with primitive RKNN methods
4. **Unit test suite** for primitive functions
5. **Updated documentation** reflecting primitive-only scope

## Success Metrics ✅
- Successfully load and run `.rknn` vision models via primitive API
- Each RKNN wrapper function works as direct 1:1 mapping
- JSON-RPC methods dispatch correctly  
- Clients can orchestrate multimodal workflows using primitives
- Build system supports both RKNN and RKLLM libraries
- No memory leaks or crashes

**Status**: COMPLETE - Server provides essential primitive building blocks for multimodal AI applications.