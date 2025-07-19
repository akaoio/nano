#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test suites for real model testing
#include "io/test_io_architecture.h"
#include "nano/test_nano_architecture.h"

typedef enum {
    TEST_MODE_ALL,
    TEST_MODE_IO,
    TEST_MODE_NANO
} test_mode_t;

void print_usage(const char* program_name) {
    printf("Usage: %s [--io|--nano|--help]\n", program_name);
    printf("\nOptions:\n");
    printf("  --io     Test IO layer only (requires real model)\n");
    printf("  --nano   Test NANO layer only (requires real model)\n");
    printf("  --help   Show this help message\n");
    printf("  (no args) Run all tests\n");
    printf("\nNote: All tests require real models in models/ directory\n");
}

int run_io_tests(void) {
    printf("🔧 IO LAYER TEST SUITE\n");
    printf("=====================\n");
    printf("Testing IO architecture with real models\n");
    printf("Requires: models/qwen3/model.rkllm\n\n");
    
    printf("🧪 Testing IO Architecture...\n");
    printf("─────────────────────────────\n");
    
    int result = test_io_architecture();
    
    if (result == 0) {
        printf("✅ IO Architecture: PASS\n\n");
        printf("🎉 IO TESTS PASSED! 🎉\n");
        printf("✅ IO operations working correctly\n");
        printf("✅ Handle pool management functional\n");
        printf("✅ RKLLM proxy operational\n");
        printf("✅ Worker pool and queues working\n");
    } else {
        printf("❌ IO Architecture: FAIL (%d errors)\n\n", result);
        printf("❌ IO TESTS FAILED\n");
        printf("⚠️  Check IO layer implementation\n");
        printf("🔧 Verify model loading and NPU memory\n");
    }
    
    return result;
}

int run_nano_tests(void) {
    printf("🔧 NANO LAYER TEST SUITE\n");
    printf("========================\n");
    printf("Testing NANO architecture with real models\n");
    printf("Requires: models/qwen3/model.rkllm\n\n");
    
    printf("🧪 Testing NANO Architecture...\n");
    printf("──────────────────────────────\n");
    
    int result = test_nano_architecture();
    
    if (result == 0) {
        printf("✅ NANO Architecture: PASS\n\n");
        printf("🎉 NANO TESTS PASSED! 🎉\n");
        printf("✅ System info functional\n");
        printf("✅ Resource manager working\n");
        printf("✅ Model validation operational\n");
        printf("✅ Transport layer functional\n");
    } else {
        printf("❌ NANO Architecture: FAIL (%d errors)\n\n", result);
        printf("❌ NANO TESTS FAILED\n");
        printf("⚠️  Check NANO layer implementation\n");
        printf("🔧 Verify system integration\n");
    }
    
    return result;
}

int run_all_tests(void) {
    printf("🔧 NANO PROJECT COMPLETE TEST SUITE\n");
    printf("===================================\n");
    printf("Testing both IO and NANO layers with real models\n");
    printf("Note: Integration tests removed - using real model tests only\n\n");
    
    int io_result = 0;
    int nano_result = 0;
    int total_failed = 0;
    
    // Test IO layer
    printf("🧪 Testing IO Architecture...\n");
    printf("─────────────────────────────\n");
    io_result = test_io_architecture();
    
    if (io_result == 0) {
        printf("✅ IO Architecture: PASS\n\n");
    } else {
        printf("❌ IO Architecture: FAIL (%d errors)\n\n", io_result);
        total_failed++;
    }
    
    // Test NANO layer
    printf("🧪 Testing NANO Architecture...\n");
    printf("───────────────────────────────\n");
    nano_result = test_nano_architecture();
    
    if (nano_result == 0) {
        printf("✅ NANO Architecture: PASS\n\n");
    } else {
        printf("❌ NANO Architecture: FAIL (%d errors)\n\n", nano_result);
        total_failed++;
    }
    
    // Summary
    printf("📊 TEST SUMMARY\n");
    printf("===============\n");
    printf("IO Layer: %s\n", io_result == 0 ? "PASS" : "FAIL");
    printf("NANO Layer: %s\n", nano_result == 0 ? "PASS" : "FAIL");
    printf("Total failures: %d\n", total_failed);
    
    if (total_failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n");
        printf("✅ IO layer is functional\n");
        printf("✅ NANO layer is functional\n");
        printf("✅ Real model integration working\n");
        printf("\nSystem is ready for production!\n");
    } else {
        printf("\n❌ %d LAYER(S) FAILED\n", total_failed);
        printf("⚠️  Please fix failing layers before deployment\n");
        printf("🔧 Check individual test output above for details\n");
    }
    
    return total_failed;
}

int main(int argc, char* argv[]) {
    test_mode_t mode = TEST_MODE_ALL;
    
    // Parse command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "--io") == 0) {
            mode = TEST_MODE_IO;
        } else if (strcmp(argv[1], "--nano") == 0) {
            mode = TEST_MODE_NANO;
        } else if (strcmp(argv[1], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("❌ Unknown option: %s\n\n", argv[1]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Run tests based on mode
    switch (mode) {
        case TEST_MODE_IO:
            return run_io_tests();
            
        case TEST_MODE_NANO:
            return run_nano_tests();
            
        case TEST_MODE_ALL:
        default:
            return run_all_tests();
    }
}
