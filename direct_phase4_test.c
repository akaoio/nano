#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <json-c/json.h>

// Direct function declarations (no header dependencies)
extern int error_mapping_init(void);
extern void error_mapping_shutdown(void);
extern int error_mapping_get(int rkllm_code, int* json_code, const char** json_message, const char** description);

extern int memory_tracker_init(bool log_allocations);
extern void memory_tracker_shutdown(void);
extern void* memory_tracker_malloc(size_t size, const char* file, int line);
extern void memory_tracker_free(void* ptr);

typedef struct {
    int active_allocations;
    size_t total_allocated_bytes;
    size_t peak_allocated_bytes;
    int total_allocations;
    int total_frees;
} memory_tracker_stats_t;

extern int memory_tracker_get_stats(memory_tracker_stats_t* stats);

extern int transport_recovery_init(bool auto_recovery);
extern void transport_recovery_shutdown(void);
extern bool transport_recovery_is_active(int transport_type);

// Mock RKLLM and transport type constants
#define RKLLM_SUCCESS 0
#define RKLLM_INVALID_PARAM 1
#define RKLLM_OUT_OF_MEMORY 2
#define TRANSPORT_TYPE_HTTP 1

int main() {
    int test_failures = 0;
    
    printf("=== DIRECT PHASE 4 COMPONENT TESTS ===\n\n");
    
    // TEST 1: Error Mapping
    printf("TEST 1: Error Mapping System\n");
    printf("-----------------------------\n");
    
    if (error_mapping_init() == 0) {
        printf("✅ Error mapping initialized\n");
        
        int json_code;
        const char* json_msg;
        const char* description;
        
        // Test RKLLM_SUCCESS mapping
        if (error_mapping_get(RKLLM_SUCCESS, &json_code, &json_msg, &description) == 0) {
            printf("✅ RKLLM_SUCCESS -> JSON-RPC %d: %s\n", json_code, json_msg);
            if (json_code == 0 && strstr(json_msg, "Success")) {
                printf("✅ Error mapping for success is correct\n");
            } else {
                printf("❌ ERROR: Unexpected mapping for success\n");
                test_failures++;
            }
        } else {
            printf("❌ ERROR: Failed to get error mapping for success\n");
            test_failures++;
        }
        
        // Test RKLLM_INVALID_PARAM mapping
        if (error_mapping_get(RKLLM_INVALID_PARAM, &json_code, &json_msg, &description) == 0) {
            printf("✅ RKLLM_INVALID_PARAM -> JSON-RPC %d: %s\n", json_code, json_msg);
            if (json_code == -32602 && strstr(json_msg, "parameter")) {
                printf("✅ Error mapping for invalid param is correct\n");
            } else {
                printf("❌ ERROR: Unexpected mapping for invalid param\n");
                test_failures++;
            }
        } else {
            printf("❌ ERROR: Failed to get error mapping for invalid param\n");
            test_failures++;
        }
        
        error_mapping_shutdown();
        printf("✅ Error mapping shutdown complete\n");
    } else {
        printf("❌ ERROR: Failed to initialize error mapping\n");
        test_failures++;
    }
    
    // TEST 2: Memory Tracker  
    printf("\nTEST 2: Memory Tracker System\n");
    printf("-----------------------------\n");
    
    if (memory_tracker_init(false) == 0) {
        printf("✅ Memory tracker initialized\n");
        
        // Test allocation tracking
        void* ptr1 = memory_tracker_malloc(1024, __FILE__, __LINE__);
        void* ptr2 = memory_tracker_malloc(512, __FILE__, __LINE__);
        
        if (ptr1 && ptr2) {
            printf("✅ Allocated memory blocks: 1024 + 512 = 1536 bytes\n");
            
            // Get stats
            memory_tracker_stats_t stats;
            if (memory_tracker_get_stats(&stats) == 0) {
                printf("✅ Memory stats: %d blocks, %zu bytes allocated\n", 
                       stats.active_allocations, stats.total_allocated_bytes);
                       
                if (stats.active_allocations == 2 && stats.total_allocated_bytes >= 1536) {
                    printf("✅ Memory tracking working correctly\n");
                } else {
                    printf("❌ ERROR: Memory tracking stats incorrect\n");
                    test_failures++;
                }
            } else {
                printf("❌ ERROR: Failed to get memory stats\n");
                test_failures++;
            }
            
            // Free memory and check
            memory_tracker_free(ptr1);
            memory_tracker_free(ptr2);
            printf("✅ Memory freed\n");
            
            if (memory_tracker_get_stats(&stats) == 0) {
                if (stats.active_allocations == 0) {
                    printf("✅ Memory leak detection working correctly\n");
                } else {
                    printf("❌ ERROR: Memory leak detected (%d active allocations)\n", stats.active_allocations);
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
    
    // TEST 3: Transport Recovery
    printf("\nTEST 3: Transport Recovery System\n");
    printf("---------------------------------\n");
    
    if (transport_recovery_init(true) == 0) {
        printf("✅ Transport recovery initialized with auto-recovery\n");
        
        // Test recovery status
        if (!transport_recovery_is_active(TRANSPORT_TYPE_HTTP)) {
            printf("✅ HTTP transport not in recovery (normal state)\n");
        } else {
            printf("❌ WARNING: HTTP transport unexpectedly in recovery\n");
        }
        
        transport_recovery_shutdown();
        printf("✅ Transport recovery shutdown complete\n");
    } else {
        printf("❌ ERROR: Failed to initialize transport recovery\n");
        test_failures++;
    }
    
    // RESULTS
    printf("\n=== PHASE 4 VALIDATION RESULTS ===\n");
    if (test_failures == 0) {
        printf("🎉 ALL TESTS PASSED! Phase 4 implementations working correctly.\n");
        printf("✅ Error mapping: RKLLM -> JSON-RPC conversion functional\n");
        printf("✅ Memory tracking: Allocation/leak detection functional\n"); 
        printf("✅ Transport recovery: Recovery system initialization functional\n");
        return 0;
    } else {
        printf("❌ %d TEST(S) FAILED! Phase 4 needs attention.\n", test_failures);
        return 1;
    }
}