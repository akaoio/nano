#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test suites
#include "io/test_io_architecture.h"
#include "nano/test_nano_architecture.h"
#include "integration/test_qwenvl.h"
#include "integration/test_lora.h"

// Test suite results
typedef struct {
    const char* name;
    int (*run_test)(void);
    int result;
} test_t;

int main(void) {
    printf("🔧 NANO PROJECT TEST SUITE\n");
    printf("==========================\n");
    printf("Testing according to new architecture (newtree.md)\n");
    printf("Build target: io, nano, test\n\n");
    
    // Define test suites
    test_t suites[] = {
        {"IO Architecture", test_io_architecture, 0},
        {"NANO Architecture", test_nano_architecture, 0},
        {"Integration - QwenVL", test_integration_qwenvl, 0},
        {"Integration - LoRA", test_integration_lora, 0},
        {NULL, NULL, 0}
    };
    
    int total_tests = 0;
    int failed_tests = 0;
    
    // Run test suites
    for (int i = 0; suites[i].name != NULL; i++) {
        total_tests++;
        printf("🧪 Testing %s...\n", suites[i].name);
        printf("─────────────────────────────────\n");
        
        suites[i].result = suites[i].run_test();
        
        if (suites[i].result == 0) {
            printf("✅ %s: PASS\n\n", suites[i].name);
        } else {
            printf("❌ %s: FAIL (%d errors)\n\n", suites[i].name, suites[i].result);
            failed_tests++;
        }
    }
    
    // Summary
    printf("📊 TEST SUMMARY\n");
    printf("===============\n");
    printf("Total suites: %d\n", total_tests);
    printf("Passed: %d\n", total_tests - failed_tests);
    printf("Failed: %d\n", failed_tests);
    
    if (failed_tests == 0) {
        printf("\n🎉 ALL TESTS PASSED! 🎉\n");
        printf("✅ Build system is working correctly\n");
        printf("✅ IO layer is functional\n");
        printf("✅ Nano layer is functional\n");
        printf("✅ Integration tests passed\n");
        printf("\nSystem is ready for production!\n");
    } else {
        printf("\n❌ %d TEST SUITE(S) FAILED\n", failed_tests);
        printf("⚠️  Please fix failing tests before deployment\n");
        printf("🔧 Check individual test output above for details\n");
    }
    
    return failed_tests;
}
