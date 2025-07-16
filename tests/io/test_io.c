#include "test_io.h"

int run_all_io_tests(void) {
    printf("🔍 Testing IO layer - Real Model Validation & Response Testing\n");
    printf("=============================================================\n\n");
    
    // Initial system cleanup
    printf("🧹 Initial system memory cleanup...\n");
    system_force_gc();
    system_free_memory();
    
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
    
    // Test 3: Test model loading + inference (combined)
    printf("Test 3a: QwenVL full test... ");
    test_result = test_qwenvl_full();
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    printf("Test 3b: LoRA full test... ");
    test_result = test_lora_full();
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 4: Test error cases
    printf("Test 4: Error handling... ");
    test_result = test_error_cases();
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 5: Cleanup (no specific handles needed now)
    printf("Test 5: Cleanup... ");
    test_result = test_cleanup(0, 0);
    if (test_result == 0) {
        printf("✅ PASS\n");
    } else {
        printf("❌ FAIL\n");
    }
    failed += test_result;
    
    // Test 6: Shutdown
    printf("\n🛑 Testing shutdown...\n");
    io_shutdown();
    printf("✅ IO shutdown completed\n");
    
    // Print summary
    printf("\n📊 Test Summary:\n");
    printf("===============\n");
    if (failed == 0) {
        printf("✅ All tests passed!\n");
    } else {
        printf("❌ %d test(s) failed\n", failed);
    }
    
    return failed;
}
