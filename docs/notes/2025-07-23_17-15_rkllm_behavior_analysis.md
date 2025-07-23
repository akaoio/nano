# RKLLM Behavior Analysis - Synchronous vs Asynchronous

**Date:** 2025-07-23 17:15  
**Subject:** Investigation of weird `rkllm_run` behavior

## Key Discovery

**The synchronous `rkllm_run` behaves EXACTLY like `rkllm_run_async` - it calls the callback with streaming tokens!**

This is highly unusual and suggests one of the following:

## Evidence

1. **From server logs** (test_sync_callback.py):
```
[DEBUG] Callback called - state: 0, text: Hello
[DEBUG] Callback called - state: 0, text: !
[DEBUG] Callback called - state: 0, text:  I
[DEBUG] Callback called - state: 0, text: 'm
...
[DEBUG] Callback called - state: 2, text: NULL
[DEBUG] rkllm_run returned: 0
```

2. **Both `rkllm_run` and `rkllm_run_async` use the same callback mechanism**

3. **The `is_async` parameter in RKLLMParam appears to control this behavior**

## Analysis of Implementation

### 1. Server ignores `is_async` parameter
- The server routes based on JSON-RPC method name:
  - `"rkllm.run"` → calls `rkllm_run()`
  - `"rkllm.run_async"` → calls `rkllm_run_async()`
- The `is_async` parameter is parsed and passed to `rkllm_init()` but ignored for routing

### 2. Possible RKLLM Library Behavior
The RKLLM library might be designed such that:
- When `is_async=true` in init params, BOTH `rkllm_run` and `rkllm_run_async` behave asynchronously
- When `is_async=false` in init params, BOTH functions behave synchronously
- The actual function called (`rkllm_run` vs `rkllm_run_async`) may not matter

### 3. Current Server Implementation
- Sets streaming context for BOTH sync and async calls
- Both receive callbacks with streaming tokens
- The only difference might be in blocking behavior

## Hypothesis

**The RKLLM library's `is_async` parameter in `rkllm_init()` controls the actual behavior**, not which function you call:

1. **If `is_async=true`**: Both `rkllm_run` and `rkllm_run_async` stream tokens via callbacks
2. **If `is_async=false`**: Both functions might block without callbacks (untested)

## Server Design Issues

1. **Hardcoded routing**: Server ignores `is_async` and routes based on method name
2. **No respect for init parameter**: The `is_async` flag during init is not considered
3. **Misleading API**: Two different methods that behave the same way

## Recommendations

1. **Test with `is_async=false`** to see if callbacks stop firing
2. **Consider respecting `is_async` parameter** from init instead of method routing
3. **Document this unusual behavior** clearly for clients

## Current Status

The server works but doesn't follow expected semantics:
- `rkllm.run` is supposed to be synchronous but streams like async
- `rkllm.run_async` supposedly async but `rkllm_run_async()` crashes
- Both methods currently use synchronous `rkllm_run()` with streaming callbacks

This explains why the user said "it is very weird that rkllm_run behaves just like rkllm_run_async"!