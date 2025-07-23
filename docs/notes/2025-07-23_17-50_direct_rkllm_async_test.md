# Direct RKLLM Async Test Investigation

**Date:** 2025-07-23 17:50  
**Subject:** Creating isolated C test to verify if rkllm_run_async is broken in the library

## Context

From DESIGN.md analysis:
- System is **designed** around `rkllm.run_async` working properly (lines 85-91)
- Expected flow: Client → JSON-RPC → `rkllm_run_async` → callbacks → streaming response
- Current issue: `rkllm_run_async` hangs indefinitely in our server implementation

## Research Summary

- No specific GitHub issues found about `rkllm_run_async` hanging
- No clear usage examples in official repo
- Release v1.2.1 mentions "Optimize the callback function to support pausing inference"
- Third-party implementations don't show clear async usage

## Test Plan

Create minimal C test (`sandbox/test_rkllm_async_direct.c`) to:

1. **Isolate the issue**: Test RKLLM library directly without server overhead
2. **Compare sync vs async**: Run identical operations with both APIs
3. **Verify callbacks**: Check if callbacks work properly with async
4. **Test parameters**: Verify if specific parameters cause the hang
5. **Library behavior**: Document actual vs expected behavior

## Expected Outcomes

- **If async works**: Issue is in our server implementation
- **If async hangs**: Issue is in RKLLM library itself
- **If callbacks differ**: Callback mechanism issue between sync/async

## Next Steps

1. Create direct C test
2. Compile and run isolated test
3. Document findings
4. Determine if library bug or implementation issue