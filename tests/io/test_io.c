#include "test_io.h"

int run_all_io_tests(void) {
    printf("🔍 Testing IO layer - Real Model Validation & Response Testing\n");
    printf("=============================================================\n\n");
    
    uint32_t qwenvl_handle = 0;
    uint32_t lora_handle = 0;
    int failed = 0;
    int test_result = 0;
    
    // Test 1: Initialize IO
    printf("Test 1: IO initialization... ");
    test_result = test_io_init();
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 2: Check model files exist
    printf("Test 2: Model file validation... ");
    test_result = test_model_files();
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 3: Test actual model loading
    printf("Test 3a: QwenVL model loading... ");
    test_result = test_qwenvl_loading(&qwenvl_handle);
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    printf("Test 3b: LoRA model loading... ");
    test_result = test_lora_loading(&lora_handle);
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 4: Test actual inference
    printf("Test 4a: QwenVL inference... ");
    test_result = test_qwenvl_inference(qwenvl_handle);
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    printf("Test 4b: LoRA inference... ");
    test_result = test_lora_inference(lora_handle);
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 5: Test error cases
    printf("Test 5: Error handling... ");
    test_result = test_error_cases();
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 6: Cleanup
    printf("Test 6: Cleanup... ");
    test_result = test_cleanup(qwenvl_handle, lora_handle);
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 7: Shutdown
    printf("\n🛑 Testing shutdown...\n");
    io_shutdown();
    printf("✅ IO shutdown completed\n");
    
    // Print summary
    print_test_summary(failed);
    
    return failed;
}
