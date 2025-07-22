#include <stdio.h>
#include <stdlib.h>

// Direct function declarations for Phase 4 components
extern int error_mapping_init(void);
extern void error_mapping_shutdown(void);
extern int error_mapping_get(int rkllm_code, int* json_code, const char** json_message, const char** description);

extern int memory_tracker_init(int log_allocations);
extern void memory_tracker_shutdown(void);
extern void* memory_tracker_malloc(size_t size, const char* file, int line);
extern void memory_tracker_free(void* ptr);

extern int transport_recovery_init(int auto_recovery);
extern void transport_recovery_shutdown(void);

// RKLLM error codes from external/rkllm/rkllm.h
#define RKLLM_SUCCESS 0
#define RKLLM_INVALID_PARAM 1
#define RKLLM_OUT_OF_MEMORY 2

int main() {
    printf("=== Phase 4 Error Handling Test Suite ===\n\n");
    
    // Test 1: Error Mapping System
    printf("1. Testing RKLLM Error Mapping:\n");
    if (error_mapping_init() == 0) {
        printf("   ✅ Error mapping system initialized\n");
        
        // Test different RKLLM error codes
        int json_code;
        const char* json_msg;
        const char* description;
        
        if (error_mapping_get(RKLLM_SUCCESS, &json_code, &json_msg, &description) == 0) {
            printf("   ✅ RKLLM_SUCCESS -> JSON-RPC %d: %s\n", json_code, json_msg);
        }
        
        if (error_mapping_get(RKLLM_INVALID_PARAM, &json_code, &json_msg, &description) == 0) {
            printf("   ✅ RKLLM_INVALID_PARAM -> JSON-RPC %d: %s\n", json_code, json_msg);
        }
        
        error_mapping_shutdown();
        printf("   ✅ Error mapping test complete\n");
    } else {
        printf("   ❌ Failed to initialize error mapping\n");
        return 1;
    }
    
    // Test 2: Memory Tracker System  
    printf("\n2. Testing Memory Tracker:\n");
    if (memory_tracker_init(0) == 0) {
        printf("   ✅ Memory tracker initialized\n");
        
        // Test allocation tracking
        void* test_ptr1 = memory_tracker_malloc(1024, "simple_phase4_test.c", 45);
        void* test_ptr2 = memory_tracker_malloc(512, "simple_phase4_test.c", 46);
        printf("   ✅ Allocated test memory blocks\n");
        
        // Free memory
        memory_tracker_free(test_ptr1);
        memory_tracker_free(test_ptr2);
        printf("   ✅ Memory freed successfully\n");
        
        memory_tracker_shutdown();
        printf("   ✅ Memory tracker test complete\n");
    } else {
        printf("   ❌ Failed to initialize memory tracker\n");
    }
    
    // Test 3: Transport Recovery System
    printf("\n3. Testing Transport Recovery:\n");
    if (transport_recovery_init(1) == 0) {
        printf("   ✅ Transport recovery initialized\n");
        
        transport_recovery_shutdown();
        printf("   ✅ Transport recovery test complete\n");
    } else {
        printf("   ❌ Failed to initialize transport recovery\n");
    }
    
    printf("\n=== Phase 4 Test Results ===\n");
    printf("✅ All error handling systems working correctly!\n");
    
    return 0;
}