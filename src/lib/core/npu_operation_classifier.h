#ifndef NPU_OPERATION_CLASSIFIER_H
#define NPU_OPERATION_CLASSIFIER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    OPERATION_INSTANT,      // No NPU - process immediately
    OPERATION_NPU_QUEUE,    // Needs NPU - queue for single instance processing
    OPERATION_STREAMING     // Async streaming with NPU
} npu_operation_type_t;

// Classification based on NPU requirements
typedef struct {
    const char* method_name;
    npu_operation_type_t type;
    bool requires_npu_memory;
    int estimated_duration_ms;
} npu_operation_meta_t;

// NPU Operation Registry - based on hardware analysis
static const npu_operation_meta_t g_npu_operation_registry[] = {
    // Instant processing - No NPU memory needed (15+ functions from RKLLM header)
    {"rkllm_get_functions", OPERATION_INSTANT, false, 10},          // Custom function
    {"rkllm_get_constants", OPERATION_INSTANT, false, 5},           // Custom function
    {"rkllm_createDefaultParam", OPERATION_INSTANT, false, 1},      // RKLLM function
    {"rkllm_destroy", OPERATION_INSTANT, false, 100},               // RKLLM function
    {"rkllm_abort", OPERATION_INSTANT, false, 50},                  // RKLLM function
    {"rkllm_is_running", OPERATION_INSTANT, false, 1},              // RKLLM function
    {"rkllm_clear_kv_cache", OPERATION_INSTANT, false, 20},         // RKLLM function
    {"rkllm_get_kv_cache_size", OPERATION_INSTANT, false, 5},       // RKLLM function
    {"rkllm_set_chat_template", OPERATION_INSTANT, false, 10},      // RKLLM function
    {"rkllm_set_function_tools", OPERATION_INSTANT, false, 15},     // RKLLM function
    {"rkllm_set_cross_attn_params", OPERATION_INSTANT, false, 10},  // RKLLM function
    {"rkllm_release_prompt_cache", OPERATION_INSTANT, false, 50},   // RKLLM function
    
    // NPU Queue - Requires exclusive NPU memory access (5 functions from analysis)
    {"rkllm_init", OPERATION_NPU_QUEUE, true, 45000},               // 45s - Model loading
    {"rkllm_run", OPERATION_NPU_QUEUE, true, 5000},                 // 5s - Synchronous inference
    {"rkllm_run_async", OPERATION_STREAMING, true, -1},             // Streaming inference
    {"rkllm_load_lora", OPERATION_NPU_QUEUE, true, 2000},           // 2s - LoRA adapter loading
    {"rkllm_load_prompt_cache", OPERATION_NPU_QUEUE, true, 1000},   // 1s - Prompt cache loading (uncertain)
    
    // Sentinel
    {NULL, OPERATION_INSTANT, false, 0}
};

// Function declarations
npu_operation_type_t npu_classify_operation(const char* method_name);
int get_estimated_wait_time(const char* method_name);
bool requires_npu_memory(const char* method_name);
const char* npu_operation_type_to_string(npu_operation_type_t type);

#endif // NPU_OPERATION_CLASSIFIER_H