# Implementing rkllm.init Function - 2025-01-23 14:20:00

## Status: 🔄 IMPLEMENTING RKLLM.INIT

### Current Achievement Summary
From previous notes, the foundation is solid:
- ✅ JSON-RPC 2.0 server fully functional
- ✅ Ultra-modular architecture with 1 function per file
- ✅ Socket cleanup robust and tested
- ✅ `rkllm.createDefaultParam` working perfectly
- ✅ Connection management complete
- ✅ Real client-server communication proven

### Next Critical Implementation: rkllm.init

According to DESIGN.md lines 155-156:
- `rkllm.init` → `rkllm_init(LLMHandle* handle, RKLLMParam* param, LLMResultCallback callback)`

### Implementation Plan

1. **Create call_rkllm_init function** following ultra-modular rules:
   - `src/rkllm/call_rkllm_init/call_rkllm_init.c`
   - `src/rkllm/call_rkllm_init/call_rkllm_init.h`

2. **Function Signature (from DESIGN.md)**:
   ```c
   json_object* call_rkllm_init(json_object* params);
   ```

3. **JSON-RPC Request Format (DESIGN.md lines 174-203)**:
   ```json
   {
     "jsonrpc": "2.0",
     "id": 2,
     "method": "rkllm.init", 
     "params": [
       {
         "model_path": "/home/x/Projects/nano/models/qwen3/model.rkllm",
         "max_context_len": 512,
         "max_new_tokens": 256,
         // ... all RKLLMParam fields
       }
     ]
   }
   ```

4. **Need to implement**:
   - JSON to RKLLMParam conversion (DESIGN.md line 354)
   - Global LLMHandle management (server maintains state)
   - Callback setup for streaming
   - Error handling for model loading

5. **Server State Management**:
   - Only ONE model can be loaded at a time (DESIGN.md lines 431-434)
   - Server is "dumb worker" - no automatic behaviors (lines 437-440)
   - Model path comes from client request, not environment

### Test Model Available
- `/home/x/Projects/nano/models/qwen3/model.rkllm` (PROMPT.md line 9)
- Must not hardcode in server
- Client provides path in request

### Expected Behavior
1. Client sends `rkllm.init` with model params
2. Server converts JSON to RKLLMParam structure
3. Server calls `rkllm_init()` with callback
4. Server responds with success/error
5. Server maintains LLMHandle for future requests

Following PROMPT.md: "Always run test with programmatic technocratic strong results before moving on to next implementations."

Let's implement this step by step, test thoroughly, then move to `rkllm.run_async`.