# Implementing Missing RKLLM Functions

**Date:** 2025-07-23 18:10  
**Task:** Implement all missing RKLLM functions to complete the 1:1 API mapping

## Current Implementation Status

### ✅ IMPLEMENTED (6/15 functions - 40%)
- `rkllm.createDefaultParam` → `rkllm_createDefaultParam()`
- `rkllm.init` → `rkllm_init()`
- `rkllm.run` → `rkllm_run()` (works perfectly with streaming)
- `rkllm.run_async` → `rkllm_run_async()` (implemented but library has bugs)
- `rkllm.is_running` → `rkllm_is_running()`
- `rkllm.abort` → `rkllm_abort()`

### ❌ MISSING (9/15 functions - 60%)
1. `rkllm.destroy` → `rkllm_destroy()`
2. `rkllm.load_lora` → `rkllm_load_lora()`
3. `rkllm.load_prompt_cache` → `rkllm_load_prompt_cache()`
4. `rkllm.release_prompt_cache` → `rkllm_release_prompt_cache()`
5. `rkllm.clear_kv_cache` → `rkllm_clear_kv_cache()`
6. `rkllm.get_kv_cache_size` → `rkllm_get_kv_cache_size()`
7. `rkllm.set_chat_template` → `rkllm_set_chat_template()`
8. `rkllm.set_function_tools` → `rkllm_set_function_tools()`
9. `rkllm.set_cross_attn_params` → `rkllm_set_cross_attn_params()`

## Implementation Plan

Following DESIGN.md ultra-modular architecture:
- Each function gets its own directory: `call_<function_name>/`
- Each directory contains: `call_<function_name>.c` and `call_<function_name>.h`
- 1:1 parameter mapping from JSON-RPC to RKLLM library calls
- Add all functions to `handle_request.c` dispatcher

## Architecture Requirements

1. **Rule #1**: One function per file
2. **Rule #2**: No two functions in one file  
3. **Rule #3**: Directory/file naming: `<name>/<name>.<c|h>`
4. **1:1 RKLLM Mapping**: Exact parameter names, types, and structures
5. **JSON-RPC 2.0**: Standard request/response format

## Implementation Order

1. Resource management functions (destroy, cache management)
2. Advanced features (LoRA, chat templates, function calling)
3. Cross-attention support
4. Update request handler
5. Comprehensive testing

## Testing Requirements

- Test each function individually
- Test with real models at `/home/x/Projects/nano/models/qwen3/model.rkllm`  
- Test LoRA with `/home/x/Projects/nano/models/lora/model.rkllm` and `/home/x/Projects/nano/models/lora/lora.rkllm`
- Verify JSON-RPC request/response format
- Test error handling and edge cases

## Expected Completion

By end of session, all 9 missing functions implemented and tested, bringing total coverage to 15/15 (100%).