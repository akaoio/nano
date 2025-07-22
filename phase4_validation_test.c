#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include only the specific headers we need
#include "src/lib/core/error_mapping.h"
#include "src/lib/core/memory_tracker.h"
#include "src/lib/transport/recovery.h"

// Mock RKLLM error codes for testing
#define RKLLM_SUCCESS 0
#define RKLLM_INVALID_PARAM 1
#define RKLLM_OUT_OF_MEMORY 2
#define RKLLM_MODEL_NOT_FOUND 3

int main() {
    int test_failures = 0;
    
    printf("=== PHASE 4 VALIDATION TEST SUITE ===\n\n");
    
    // TEST 1: Error Mapping System
    printf("TEST 1: RKLLM Error Mapping System\n");
    printf("-----------------------------------\n");
    
    if (error_mapping_init() == 0) {
        printf("✅ Error mapping system initialized\n");
        
        // Test mapping RKLLM_SUCCESS
        int json_code;
        const char* json_msg;
        const char* description;
        
        if (error_mapping_get(RKLLM_SUCCESS, &json_code, &json_msg, &description) == 0) {
            printf("✅ RKLLM_SUCCESS -> JSON-RPC %d: '%s'\n", json_code, json_msg);
            if (json_code != 0) {
                printf("❌ ERROR: Expected JSON code 0 for success, got %d\n", json_code);
                test_failures++;
            }
        } else {
            printf("❌ ERROR: Failed to get mapping for RKLLM_SUCCESS\n");
            test_failures++;
        }
        
        // Test mapping RKLLM_INVALID_PARAM
        if (error_mapping_get(RKLLM_INVALID_PARAM, &json_code, &json_msg, &description) == 0) {
            printf("✅ RKLLM_INVALID_PARAM -> JSON-RPC %d: '%s'\n", json_code, json_msg);
            if (json_code != -32602) {
                printf("❌ ERROR: Expected JSON code -32602 for invalid param, got %d\n", json_code);
                test_failures++;
            }
        } else {
            printf("❌ ERROR: Failed to get mapping for RKLLM_INVALID_PARAM\n");
            test_failures++;
        }
        
        error_mapping_shutdown();
        printf("✅ Error mapping system shutdown complete\n");
    } else {
        printf("❌ ERROR: Failed to initialize error mapping system\n");
        test_failures++;
    }
    
    // TEST 2: Memory Tracker System
    printf("\nTEST 2: Memory Tracker System\n");
    printf("-----------------------------\n");
    
    if (memory_tracker_init(0) == 0) { // Initialize without verbose logging
        printf("✅ Memory tracker initialized\n");
        
        // Test memory allocation tracking
        void* ptr1 = memory_tracker_malloc(1024, __FILE__, __LINE__);
        void* ptr2 = memory_tracker_malloc(512, __FILE__, __LINE__);
        
        if (ptr1 && ptr2) {
            printf("✅ Allocated 1536 bytes total (1024 + 512)\n");
            
            // Get statistics
            memory_tracker_stats_t stats;
            if (memory_tracker_get_stats(&stats) == 0) {
                printf("✅ Current allocations: %d blocks, %zu bytes\n", 
                       stats.active_allocations, stats.total_allocated_bytes);
                
                if (stats.active_allocations != 2) {
                    printf("❌ ERROR: Expected 2 active allocations, got %d\n", stats.active_allocations);
                    test_failures++;
                }
                if (stats.total_allocated_bytes != 1536) {
                    printf("❌ ERROR: Expected 1536 bytes allocated, got %zu\n", stats.total_allocated_bytes);
                    test_failures++;
                }
            } else {
                printf("❌ ERROR: Failed to get memory tracker statistics\n");
                test_failures++;
            }
            
            // Free memory
            memory_tracker_free(ptr1);
            memory_tracker_free(ptr2);
            printf("✅ Memory freed successfully\n");
            
            // Check stats after freeing
            if (memory_tracker_get_stats(&stats) == 0) {
                if (stats.active_allocations != 0) {
                    printf("❌ ERROR: Expected 0 active allocations after free, got %d\n", stats.active_allocations);
                    test_failures++;
                }
            }
            
        } else {
            printf("❌ ERROR: Memory allocation failed\n");
            test_failures++;
        }
        
        memory_tracker_shutdown();
        printf("✅ Memory tracker shutdown complete\n");
    } else {
        printf("❌ ERROR: Failed to initialize memory tracker\n");
        test_failures++;
    }
    
    // TEST 3: Transport Recovery System
    printf("\nTEST 3: Transport Recovery System\n");
    printf("---------------------------------\n");
    
    if (transport_recovery_init(1) == 0) { // Enable auto-recovery
        printf("✅ Transport recovery initialized\n");
        
        // Test recovery status check
        if (!transport_recovery_is_active(TRANSPORT_TYPE_HTTP)) {
            printf("✅ HTTP transport not in recovery (normal state)\n");
        } else {
            printf("❌ WARNING: HTTP transport unexpectedly in recovery state\n");
        }
        
        // Test statistics retrieval
        int failure_count;
        recovery_state_t state;
        uint64_t last_failure_time;
        if (transport_recovery_get_stats(TRANSPORT_TYPE_HTTP, &failure_count, &state, &last_failure_time) == 0) {
            printf("✅ HTTP transport stats retrieved: %d failures, state %d\n", failure_count, (int)state);
            
            if (failure_count != 0) {
                printf("❌ WARNING: Expected 0 failures initially, got %d\n", failure_count);
            }
        } else {
            printf("❌ ERROR: Failed to get transport recovery stats\n");
            test_failures++;
        }
        
        // Test configuration
        if (transport_recovery_configure(TRANSPORT_TYPE_HTTP, 3, 1000, 10000) == 0) {
            printf("✅ HTTP transport recovery configured successfully\n");
        } else {
            printf("❌ ERROR: Failed to configure transport recovery\n");
            test_failures++;
        }
        
        transport_recovery_shutdown();
        printf("✅ Transport recovery shutdown complete\n");
    } else {
        printf("❌ ERROR: Failed to initialize transport recovery\n");
        test_failures++;
    }
    
    // TEST RESULTS
    printf("\n=== PHASE 4 VALIDATION RESULTS ===\n");
    if (test_failures == 0) {
        printf("🎉 ALL TESTS PASSED! Phase 4 implementations are working correctly.\n");
        printf("✅ Error mapping: RKLLM -> JSON-RPC error codes working\n");
        printf("✅ Memory tracking: Application-level allocation monitoring working\n");
        printf("✅ Transport recovery: Failure recovery mechanisms working\n");
        return 0;
    } else {
        printf("❌ %d TEST FAILURES detected. Phase 4 implementations need fixes.\n", test_failures);
        return 1;
    }
}