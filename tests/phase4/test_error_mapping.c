#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../src/lib/core/error_mapping.h"
#include "../../external/rkllm/rkllm.h"

int test_rkllm_error_mapping() {
    printf("Testing RKLLM Error Mapping...\n");
    
    if (!error_mapping_is_initialized()) {
        printf("FAIL: Error mapping not initialized\n");
        return -1;
    }
    
    // Test mapping of known RKLLM error codes
    struct {
        RKLLMResult rkllm_code;
        int expected_json_rpc_code;
        const char* description;
    } test_cases[] = {
        {RKLLM_RET_SUCCESS, 0, "Success case"},
        {RKLLM_RET_UNINIT, -32001, "Uninitialized error"},
        {RKLLM_RET_PARAM_ERR, -32602, "Parameter error"},
        {RKLLM_RET_ALLOC_ERR, -32000, "Allocation error"},
        {RKLLM_RET_READ_ERR, -32001, "Read error"},
        {RKLLM_RET_WRITE_ERR, -32001, "Write error"}
    };
    
    int test_count = sizeof(test_cases) / sizeof(test_cases[0]);
    int failures = 0;
    
    for (int i = 0; i < test_count; i++) {
        int mapped_code = map_rkllm_error_to_json_rpc(test_cases[i].rkllm_code);
        
        if (mapped_code != test_cases[i].expected_json_rpc_code) {
            printf("FAIL: %s - Expected %d, got %d\n", 
                   test_cases[i].description, 
                   test_cases[i].expected_json_rpc_code, 
                   mapped_code);
            failures++;
        } else {
            printf("PASS: %s\n", test_cases[i].description);
        }
    }
    
    if (failures == 0) {
        printf("PASS: RKLLM Error Mapping (all %d test cases passed)\n", test_count);
        return 0;
    } else {
        printf("FAIL: RKLLM Error Mapping (%d/%d test cases failed)\n", failures, test_count);
        return -1;
    }
}

int main() {
    printf("=== Phase 4 Error Mapping Tests ===\n");
    
    // Initialize error mapping system
    if (error_mapping_init() != 0) {
        printf("FAIL: Could not initialize error mapping system\n");
        return 1;
    }
    
    int failures = 0;
    
    if (test_rkllm_error_mapping() != 0) {
        failures++;
    }
    
    // Cleanup
    error_mapping_cleanup();
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: 1\n");
    printf("Failures: %d\n", failures);
    printf("Success: %d\n", 1 - failures);
    
    return failures > 0 ? 1 : 0;
}