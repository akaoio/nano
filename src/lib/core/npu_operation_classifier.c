#include "npu_operation_classifier.h"
#include <string.h>
#include <stdio.h>

npu_operation_type_t npu_classify_operation(const char* method_name) {
    if (!method_name) {
        return OPERATION_INSTANT;
    }
    
    for (int i = 0; g_npu_operation_registry[i].method_name != NULL; i++) {
        if (strcmp(method_name, g_npu_operation_registry[i].method_name) == 0) {
            printf("🔍 NPU Classifier: %s -> %s (NPU: %s, Duration: %dms)\n", 
                   method_name, 
                   npu_operation_type_to_string(g_npu_operation_registry[i].type),
                   g_npu_operation_registry[i].requires_npu_memory ? "YES" : "NO",
                   g_npu_operation_registry[i].estimated_duration_ms);
            return g_npu_operation_registry[i].type;
        }
    }
    
    // Unknown methods default to instant processing
    printf("⚠️  NPU Classifier: Unknown method '%s' - defaulting to INSTANT\n", method_name);
    return OPERATION_INSTANT;
}

int get_estimated_wait_time(const char* method_name) {
    if (!method_name) {
        return 0;
    }
    
    for (int i = 0; g_npu_operation_registry[i].method_name != NULL; i++) {
        if (strcmp(method_name, g_npu_operation_registry[i].method_name) == 0) {
            return g_npu_operation_registry[i].estimated_duration_ms;
        }
    }
    
    return 0;
}

bool requires_npu_memory(const char* method_name) {
    if (!method_name) {
        return false;
    }
    
    for (int i = 0; g_npu_operation_registry[i].method_name != NULL; i++) {
        if (strcmp(method_name, g_npu_operation_registry[i].method_name) == 0) {
            return g_npu_operation_registry[i].requires_npu_memory;
        }
    }
    
    return false;
}

const char* npu_operation_type_to_string(npu_operation_type_t type) {
    switch (type) {
        case OPERATION_INSTANT:
            return "INSTANT";
        case OPERATION_NPU_QUEUE:
            return "NPU_QUEUE";
        case OPERATION_STREAMING:
            return "STREAMING";
        default:
            return "UNKNOWN";
    }
}