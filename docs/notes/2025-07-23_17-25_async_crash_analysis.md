# RKLLM Async Crash Analysis

**Date:** 2025-07-23 17:25  
**Subject:** Investigation of rkllm_run_async crash

## Key Discovery

**The crash is NOT in `rkllm_run_async()` - it's a memory corruption during RKLLM initialization!**

## Evidence

### Server Log Output:
```
🖥️  I rkllm: Enabled cpus: [4, 5, 6, 7]
🖥️  I rkllm: Enabled cpus num: 4
🖥️  free(): invalid size
```

### Analysis:
1. **RKLLM initialization succeeds** - the library loads and shows CPU info
2. **Crash occurs during cleanup** - `free(): invalid size` happens after init
3. **The crash is in memory deallocation**, not the async call

## Root Cause

The issue is likely in `call_rkllm_init.c` where we clean up allocated strings:

```c
// Clean up allocated strings
if (rkllm_param.model_path) free((void*)rkllm_param.model_path);
if (rkllm_param.img_start) free((void*)rkllm_param.img_start);
if (rkllm_param.img_end) free((void*)rkllm_param.img_end);
if (rkllm_param.img_content) free((void*)rkllm_param.img_content);
```

## Problem Areas

1. **Double-free**: Trying to free the same memory twice
2. **Invalid free**: Trying to free memory not allocated with malloc
3. **Stack pointer free**: Trying to free stack-allocated data
4. **RKLLM library retention**: RKLLM might retain pointers to the strings

## Impact on Async Testing

This memory corruption during init means:
- We never actually reach `rkllm_run_async()`
- The server crashes before async can be tested
- All our "async crashes" are actually "init crashes"

## Next Steps

1. **Fix the memory corruption in init cleanup**
2. **Test if strings need to persist for RKLLM lifetime**
3. **Re-test async once init is stable**

## Technical Details

The crash happens with both `is_async=true` and `is_async=false`, suggesting it's not related to the async parameter but to the general cleanup code after `rkllm_init()` completes.