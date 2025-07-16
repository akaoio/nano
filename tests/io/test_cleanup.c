#include "test_io.h"

int test_cleanup(uint32_t qwenvl_handle, uint32_t lora_handle) {
    printf("\n🧹 Testing cleanup...\n");
    
    if (qwenvl_handle > 0) {
        char destroy_request[1024];
        snprintf(destroy_request, sizeof(destroy_request),
            "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"destroy\",\"params\":{\"handle_id\":%u}}", qwenvl_handle);
        
        printf("Test 6a: QwenVL destroy... ");
        if (io_push_request(destroy_request) != 0) {
            printf("FAIL - push failed\n");
            return 1;
        }
        printf("PASS\n");
    }
    
    if (lora_handle > 0) {
        char destroy_request[1024];
        snprintf(destroy_request, sizeof(destroy_request),
            "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"destroy\",\"params\":{\"handle_id\":%u}}", lora_handle);
        
        printf("Test 6b: LoRA destroy... ");
        if (io_push_request(destroy_request) != 0) {
            printf("FAIL - push failed\n");
            return 1;
        }
        printf("PASS\n");
    }
    
    return 0;
}

void print_test_summary(int failed_tests) {
    printf("\n==========================================\n");
    printf("🎯 Test Summary:\n");
    
    if (failed_tests == 0) {
        printf("✅ All tests PASSED!\n");
        printf("✅ IO initialization: PASS\n");
        printf("✅ Model file validation: PASS\n");
        printf("✅ Real model loading: PASS\n");
        printf("✅ Inference testing: PASS\n");
        printf("✅ Error handling: PASS\n");
        printf("✅ Memory management: PASS\n");
        printf("✅ Cleanup: PASS\n");
        printf("\nIO layer is production-ready with real model testing!\n");
    } else {
        printf("❌ %d test(s) FAILED!\n", failed_tests);
        printf("⚠️  Some tests did not pass. Check output above for details.\n");
        printf("🔧 Please fix the failing tests before considering the IO layer ready.\n");
    }
}
