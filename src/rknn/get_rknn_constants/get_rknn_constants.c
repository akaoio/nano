#include "get_rknn_constants.h"
#include <rknn_api.h>
#include <json-c/json.h>

json_object* get_rknn_constants(void) {
    json_object* constants = json_object_new_object();
    
    // RKNN Flags
    json_object* flags = json_object_new_object();
    json_object_object_add(flags, "RKNN_FLAG_PRIOR_HIGH", json_object_new_int(RKNN_FLAG_PRIOR_HIGH));
    json_object_object_add(flags, "RKNN_FLAG_PRIOR_MEDIUM", json_object_new_int(RKNN_FLAG_PRIOR_MEDIUM));
    json_object_object_add(flags, "RKNN_FLAG_PRIOR_LOW", json_object_new_int(RKNN_FLAG_PRIOR_LOW));
    json_object_object_add(flags, "RKNN_FLAG_ASYNC_MASK", json_object_new_int(RKNN_FLAG_ASYNC_MASK));
    json_object_object_add(flags, "RKNN_FLAG_COLLECT_PERF_MASK", json_object_new_int(RKNN_FLAG_COLLECT_PERF_MASK));
    json_object_object_add(flags, "RKNN_FLAG_MEM_ALLOC_OUTSIDE", json_object_new_int(RKNN_FLAG_MEM_ALLOC_OUTSIDE));
    json_object_object_add(flags, "RKNN_FLAG_SHARE_WEIGHT_MEM", json_object_new_int(RKNN_FLAG_SHARE_WEIGHT_MEM));
    json_object_object_add(flags, "RKNN_FLAG_FENCE_IN_OUTSIDE", json_object_new_int(RKNN_FLAG_FENCE_IN_OUTSIDE));
    json_object_object_add(flags, "RKNN_FLAG_FENCE_OUT_OUTSIDE", json_object_new_int(RKNN_FLAG_FENCE_OUT_OUTSIDE));
    json_object_object_add(flags, "RKNN_FLAG_COLLECT_MODEL_INFO_ONLY", json_object_new_int(RKNN_FLAG_COLLECT_MODEL_INFO_ONLY));
    json_object_object_add(flags, "RKNN_FLAG_INTERNAL_ALLOC_OUTSIDE", json_object_new_int(RKNN_FLAG_INTERNAL_ALLOC_OUTSIDE));
    json_object_object_add(flags, "RKNN_FLAG_EXECUTE_FALLBACK_PRIOR_DEVICE_GPU", json_object_new_int(RKNN_FLAG_EXECUTE_FALLBACK_PRIOR_DEVICE_GPU));
    json_object_object_add(flags, "RKNN_FLAG_ENABLE_SRAM", json_object_new_int(RKNN_FLAG_ENABLE_SRAM));
    json_object_object_add(flags, "RKNN_FLAG_SHARE_SRAM", json_object_new_int(RKNN_FLAG_SHARE_SRAM));
    json_object_object_add(flags, "RKNN_FLAG_DISABLE_PROC_HIGH_PRIORITY", json_object_new_int(RKNN_FLAG_DISABLE_PROC_HIGH_PRIORITY));
    json_object_object_add(flags, "RKNN_FLAG_DISABLE_FLUSH_INPUT_MEM_CACHE", json_object_new_int(RKNN_FLAG_DISABLE_FLUSH_INPUT_MEM_CACHE));
    json_object_object_add(flags, "RKNN_FLAG_DISABLE_FLUSH_OUTPUT_MEM_CACHE", json_object_new_int(RKNN_FLAG_DISABLE_FLUSH_OUTPUT_MEM_CACHE));
    json_object_object_add(flags, "RKNN_FLAG_MODEL_BUFFER_ZERO_COPY", json_object_new_int(RKNN_FLAG_MODEL_BUFFER_ZERO_COPY));
    json_object_object_add(flags, "RKNN_MEM_FLAG_ALLOC_NO_CONTEXT", json_object_new_int(RKNN_MEM_FLAG_ALLOC_NO_CONTEXT));
    json_object_object_add(constants, "FLAGS", flags);
    
    // Error Codes
    json_object* errors = json_object_new_object();
    json_object_object_add(errors, "RKNN_SUCC", json_object_new_int(RKNN_SUCC));
    json_object_object_add(errors, "RKNN_ERR_FAIL", json_object_new_int(RKNN_ERR_FAIL));
    json_object_object_add(errors, "RKNN_ERR_TIMEOUT", json_object_new_int(RKNN_ERR_TIMEOUT));
    json_object_object_add(errors, "RKNN_ERR_DEVICE_UNAVAILABLE", json_object_new_int(RKNN_ERR_DEVICE_UNAVAILABLE));
    json_object_object_add(errors, "RKNN_ERR_MALLOC_FAIL", json_object_new_int(RKNN_ERR_MALLOC_FAIL));
    json_object_object_add(errors, "RKNN_ERR_PARAM_INVALID", json_object_new_int(RKNN_ERR_PARAM_INVALID));
    json_object_object_add(errors, "RKNN_ERR_MODEL_INVALID", json_object_new_int(RKNN_ERR_MODEL_INVALID));
    json_object_object_add(errors, "RKNN_ERR_CTX_INVALID", json_object_new_int(RKNN_ERR_CTX_INVALID));
    json_object_object_add(errors, "RKNN_ERR_INPUT_INVALID", json_object_new_int(RKNN_ERR_INPUT_INVALID));
    json_object_object_add(errors, "RKNN_ERR_OUTPUT_INVALID", json_object_new_int(RKNN_ERR_OUTPUT_INVALID));
    json_object_object_add(errors, "RKNN_ERR_DEVICE_UNMATCH", json_object_new_int(RKNN_ERR_DEVICE_UNMATCH));
    json_object_object_add(errors, "RKNN_ERR_INCOMPATILE_PRE_COMPILE_MODEL", json_object_new_int(RKNN_ERR_INCOMPATILE_PRE_COMPILE_MODEL));
    json_object_object_add(errors, "RKNN_ERR_INCOMPATILE_OPTIMIZATION_LEVEL_VERSION", json_object_new_int(RKNN_ERR_INCOMPATILE_OPTIMIZATION_LEVEL_VERSION));
    json_object_object_add(errors, "RKNN_ERR_TARGET_PLATFORM_UNMATCH", json_object_new_int(RKNN_ERR_TARGET_PLATFORM_UNMATCH));
    json_object_object_add(constants, "ERRORS", errors);
    
    // Query Commands
    json_object* queries = json_object_new_object();
    json_object_object_add(queries, "RKNN_QUERY_IN_OUT_NUM", json_object_new_int(RKNN_QUERY_IN_OUT_NUM));
    json_object_object_add(queries, "RKNN_QUERY_INPUT_ATTR", json_object_new_int(RKNN_QUERY_INPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_OUTPUT_ATTR", json_object_new_int(RKNN_QUERY_OUTPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_PERF_DETAIL", json_object_new_int(RKNN_QUERY_PERF_DETAIL));
    json_object_object_add(queries, "RKNN_QUERY_PERF_RUN", json_object_new_int(RKNN_QUERY_PERF_RUN));
    json_object_object_add(queries, "RKNN_QUERY_SDK_VERSION", json_object_new_int(RKNN_QUERY_SDK_VERSION));
    json_object_object_add(queries, "RKNN_QUERY_MEM_SIZE", json_object_new_int(RKNN_QUERY_MEM_SIZE));
    json_object_object_add(queries, "RKNN_QUERY_CUSTOM_STRING", json_object_new_int(RKNN_QUERY_CUSTOM_STRING));
    json_object_object_add(queries, "RKNN_QUERY_NATIVE_INPUT_ATTR", json_object_new_int(RKNN_QUERY_NATIVE_INPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_NATIVE_OUTPUT_ATTR", json_object_new_int(RKNN_QUERY_NATIVE_OUTPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_NATIVE_NC1HWC2_INPUT_ATTR", json_object_new_int(RKNN_QUERY_NATIVE_NC1HWC2_INPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_NATIVE_NC1HWC2_OUTPUT_ATTR", json_object_new_int(RKNN_QUERY_NATIVE_NC1HWC2_OUTPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_NATIVE_NHWC_INPUT_ATTR", json_object_new_int(RKNN_QUERY_NATIVE_NHWC_INPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR", json_object_new_int(RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_DEVICE_MEM_INFO", json_object_new_int(RKNN_QUERY_DEVICE_MEM_INFO));
    json_object_object_add(queries, "RKNN_QUERY_INPUT_DYNAMIC_RANGE", json_object_new_int(RKNN_QUERY_INPUT_DYNAMIC_RANGE));
    json_object_object_add(queries, "RKNN_QUERY_CURRENT_INPUT_ATTR", json_object_new_int(RKNN_QUERY_CURRENT_INPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_CURRENT_OUTPUT_ATTR", json_object_new_int(RKNN_QUERY_CURRENT_OUTPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_CURRENT_NATIVE_INPUT_ATTR", json_object_new_int(RKNN_QUERY_CURRENT_NATIVE_INPUT_ATTR));
    json_object_object_add(queries, "RKNN_QUERY_CURRENT_NATIVE_OUTPUT_ATTR", json_object_new_int(RKNN_QUERY_CURRENT_NATIVE_OUTPUT_ATTR));
    json_object_object_add(constants, "QUERIES", queries);
    
    // Tensor Types
    json_object* tensor_types = json_object_new_object();
    json_object_object_add(tensor_types, "RKNN_TENSOR_FLOAT32", json_object_new_int(RKNN_TENSOR_FLOAT32));
    json_object_object_add(tensor_types, "RKNN_TENSOR_FLOAT16", json_object_new_int(RKNN_TENSOR_FLOAT16));
    json_object_object_add(tensor_types, "RKNN_TENSOR_INT8", json_object_new_int(RKNN_TENSOR_INT8));
    json_object_object_add(tensor_types, "RKNN_TENSOR_UINT8", json_object_new_int(RKNN_TENSOR_UINT8));
    json_object_object_add(tensor_types, "RKNN_TENSOR_INT16", json_object_new_int(RKNN_TENSOR_INT16));
    json_object_object_add(tensor_types, "RKNN_TENSOR_UINT16", json_object_new_int(RKNN_TENSOR_UINT16));
    json_object_object_add(tensor_types, "RKNN_TENSOR_INT32", json_object_new_int(RKNN_TENSOR_INT32));
    json_object_object_add(tensor_types, "RKNN_TENSOR_UINT32", json_object_new_int(RKNN_TENSOR_UINT32));
    json_object_object_add(tensor_types, "RKNN_TENSOR_INT64", json_object_new_int(RKNN_TENSOR_INT64));
    json_object_object_add(tensor_types, "RKNN_TENSOR_BOOL", json_object_new_int(RKNN_TENSOR_BOOL));
    json_object_object_add(tensor_types, "RKNN_TENSOR_INT4", json_object_new_int(RKNN_TENSOR_INT4));
    json_object_object_add(tensor_types, "RKNN_TENSOR_BFLOAT16", json_object_new_int(RKNN_TENSOR_BFLOAT16));
    json_object_object_add(constants, "TENSOR_TYPES", tensor_types);
    
    // Quantization Types
    json_object* qnt_types = json_object_new_object();
    json_object_object_add(qnt_types, "RKNN_TENSOR_QNT_NONE", json_object_new_int(RKNN_TENSOR_QNT_NONE));
    json_object_object_add(qnt_types, "RKNN_TENSOR_QNT_DFP", json_object_new_int(RKNN_TENSOR_QNT_DFP));
    json_object_object_add(qnt_types, "RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC", json_object_new_int(RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC));
    json_object_object_add(constants, "QNT_TYPES", qnt_types);
    
    // Tensor Formats
    json_object* formats = json_object_new_object();
    json_object_object_add(formats, "RKNN_TENSOR_NCHW", json_object_new_int(RKNN_TENSOR_NCHW));
    json_object_object_add(formats, "RKNN_TENSOR_NHWC", json_object_new_int(RKNN_TENSOR_NHWC));
    json_object_object_add(formats, "RKNN_TENSOR_NC1HWC2", json_object_new_int(RKNN_TENSOR_NC1HWC2));
    json_object_object_add(formats, "RKNN_TENSOR_UNDEFINED", json_object_new_int(RKNN_TENSOR_UNDEFINED));
    json_object_object_add(constants, "FORMATS", formats);
    
    // NPU Core Masks
    json_object* core_masks = json_object_new_object();
    json_object_object_add(core_masks, "RKNN_NPU_CORE_AUTO", json_object_new_int(RKNN_NPU_CORE_AUTO));
    json_object_object_add(core_masks, "RKNN_NPU_CORE_0", json_object_new_int(RKNN_NPU_CORE_0));
    json_object_object_add(core_masks, "RKNN_NPU_CORE_1", json_object_new_int(RKNN_NPU_CORE_1));
    json_object_object_add(core_masks, "RKNN_NPU_CORE_2", json_object_new_int(RKNN_NPU_CORE_2));
    json_object_object_add(core_masks, "RKNN_NPU_CORE_0_1", json_object_new_int(RKNN_NPU_CORE_0_1));
    json_object_object_add(core_masks, "RKNN_NPU_CORE_0_1_2", json_object_new_int(RKNN_NPU_CORE_0_1_2));
    json_object_object_add(core_masks, "RKNN_NPU_CORE_ALL", json_object_new_int(RKNN_NPU_CORE_ALL));
    json_object_object_add(core_masks, "RKNN_NPU_CORE_UNDEFINED", json_object_new_int(RKNN_NPU_CORE_UNDEFINED));
    json_object_object_add(constants, "CORE_MASKS", core_masks);
    
    // Memory Flags
    json_object* mem_flags = json_object_new_object();
    json_object_object_add(mem_flags, "RKNN_TENSOR_MEMORY_FLAGS_ALLOC_INSIDE", json_object_new_int(RKNN_TENSOR_MEMORY_FLAGS_ALLOC_INSIDE));
    json_object_object_add(mem_flags, "RKNN_TENSOR_MEMORY_FLAGS_FROM_FD", json_object_new_int(RKNN_TENSOR_MEMORY_FLAGS_FROM_FD));
    json_object_object_add(mem_flags, "RKNN_TENSOR_MEMORY_FLAGS_FROM_PHYS", json_object_new_int(RKNN_TENSOR_MEMORY_FLAGS_FROM_PHYS));
    json_object_object_add(mem_flags, "RKNN_TENSOR_MEMORY_FLAGS_UNKNOWN", json_object_new_int(RKNN_TENSOR_MEMORY_FLAGS_UNKNOWN));
    json_object_object_add(constants, "MEMORY_FLAGS", mem_flags);
    
    // Memory Allocation Flags
    json_object* alloc_flags = json_object_new_object();
    json_object_object_add(alloc_flags, "RKNN_FLAG_MEMORY_FLAGS_DEFAULT", json_object_new_int(RKNN_FLAG_MEMORY_FLAGS_DEFAULT));
    json_object_object_add(alloc_flags, "RKNN_FLAG_MEMORY_CACHEABLE", json_object_new_int(RKNN_FLAG_MEMORY_CACHEABLE));
    json_object_object_add(alloc_flags, "RKNN_FLAG_MEMORY_NON_CACHEABLE", json_object_new_int(RKNN_FLAG_MEMORY_NON_CACHEABLE));
    json_object_object_add(alloc_flags, "RKNN_FLAG_MEMORY_TRY_ALLOC_SRAM", json_object_new_int(RKNN_FLAG_MEMORY_TRY_ALLOC_SRAM));
    json_object_object_add(constants, "ALLOC_FLAGS", alloc_flags);
    
    // Memory Sync Modes
    json_object* sync_modes = json_object_new_object();
    json_object_object_add(sync_modes, "RKNN_MEMORY_SYNC_TO_DEVICE", json_object_new_int(RKNN_MEMORY_SYNC_TO_DEVICE));
    json_object_object_add(sync_modes, "RKNN_MEMORY_SYNC_FROM_DEVICE", json_object_new_int(RKNN_MEMORY_SYNC_FROM_DEVICE));
    json_object_object_add(sync_modes, "RKNN_MEMORY_SYNC_BIDIRECTIONAL", json_object_new_int(RKNN_MEMORY_SYNC_BIDIRECTIONAL));
    json_object_object_add(constants, "SYNC_MODES", sync_modes);
    
    // Max Values
    json_object* max_values = json_object_new_object();
    json_object_object_add(max_values, "RKNN_MAX_DIMS", json_object_new_int(RKNN_MAX_DIMS));
    json_object_object_add(max_values, "RKNN_MAX_NUM_CHANNEL", json_object_new_int(RKNN_MAX_NUM_CHANNEL));
    json_object_object_add(max_values, "RKNN_MAX_NAME_LEN", json_object_new_int(RKNN_MAX_NAME_LEN));
    json_object_object_add(max_values, "RKNN_MAX_DYNAMIC_SHAPE_NUM", json_object_new_int(RKNN_MAX_DYNAMIC_SHAPE_NUM));
    json_object_object_add(constants, "MAX_VALUES", max_values);
    
    return constants;
}