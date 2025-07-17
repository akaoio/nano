#include "../test_validation.h"
#include "model_checker/model_checker.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void test_model_check_version() {
    printf("Testing model_check_version...\n");
    
    // Version checking deprecated - test skipped
    printf("✓ model_check_version tests passed (deprecated)\n");
}

void test_model_check_compatibility() {
    printf("Testing model_check_compatibility...\n");
    
    compatibility_result_t result;
    
    // Test with non-existent file
    int ret = model_check_compatibility("nonexistent.rkllm", &result);
    assert(ret == -1);
    assert(!result.is_compatible);
    
    // Test with NULL inputs
    ret = model_check_compatibility(NULL, &result);
    assert(ret == -1);
    
    ret = model_check_compatibility("test.rkllm", NULL);
    assert(ret == -1);
    
    printf("✓ model_check_compatibility tests passed\n");
}

void test_model_check_lora_compatibility() {
    printf("Testing model_check_lora_compatibility...\n");
    
    compatibility_result_t result;
    
    // Test with non-existent files
    int ret = model_check_lora_compatibility("nonexistent.rkllm", "nonexistent_lora.rkllm", &result);
    assert(ret == -1);
    assert(!result.is_compatible);
    
    // Test with NULL inputs
    ret = model_check_lora_compatibility(NULL, "lora.rkllm", &result);
    assert(ret == -1);
    
    ret = model_check_lora_compatibility("base.rkllm", NULL, &result);
    assert(ret == -1);
    
    ret = model_check_lora_compatibility("base.rkllm", "lora.rkllm", NULL);
    assert(ret == -1);
    
    printf("✓ model_check_lora_compatibility tests passed\n");
}

int test_nano_validation(void) {
    printf("Running validation tests...\n");
    
    test_model_check_version();
    test_model_check_compatibility();
    test_model_check_lora_compatibility();
    
    printf("All validation tests passed!\n");
    return 0;
}
