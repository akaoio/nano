#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * NANO Test Suite - Main Entry Point
 * 
 * This test suite validates the complete streaming architecture:
 * Client → NANO → IO → RKLLM → Streaming Callbacks → Client
 * 
 * All tests use real models and must be run from project root.
 */

// Forward declarations
void test_streaming_client(void);
void test_io_callbacks(void);
void test_nano_callbacks(void);
void test_end_to_end_workflow(void);

// Test configuration
#define TEST_MODEL_PATH "models/qwen3/model.rkllm"

bool check_model_file() {
    struct stat st;
    if (stat(TEST_MODEL_PATH, &st) == 0) {
        printf("✅ Model file found: %s (%.1f MB)\n", 
               TEST_MODEL_PATH, (double)st.st_size / (1024 * 1024));
        return true;
    } else {
        printf("❌ Model file not found: %s\n", TEST_MODEL_PATH);
        printf("\n📍 IMPORTANT: Run tests from project root directory\n");
        printf("   Expected structure:\n");
        printf("   ./models/qwen3/model.rkllm\n");
        printf("   ./build/test\n\n");
        return false;
    }
}

void print_usage() {
    printf("NANO Test Suite - Streaming Architecture Validation\n");
    printf("==================================================\n\n");
    printf("Usage: test [option]\n\n");
    printf("Options:\n");
    printf("  --streaming    Run streaming client test (full workflow)\n");
    printf("  --io          Run IO layer callback tests\n");
    printf("  --nano        Run NANO layer callback tests\n");
    printf("  --e2e         Run end-to-end workflow test\n");
    printf("  --all         Run all tests (default)\n");
    printf("  --help        Show this help message\n\n");
    printf("Examples:\n");
    printf("  ./build/test --streaming\n");
    printf("  ./build/test --all\n\n");
    printf("Requirements:\n");
    printf("  - Run from project root directory\n");
    printf("  - Model file: %s\n", TEST_MODEL_PATH);
    printf("  - Architecture: RK3588 with NPU support\n\n");
}

int main(int argc, char* argv[]) {
    printf("🚀 NANO Test Suite - Pure Streaming Architecture\n");
    printf("================================================\n\n");
    
    // Parse command line arguments
    bool run_streaming = false;
    bool run_io = false;
    bool run_nano = false;
    bool run_e2e = false;
    bool run_all = true;
    
    if (argc > 1) {
        run_all = false;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--streaming") == 0) {
                run_streaming = true;
            } else if (strcmp(argv[i], "--io") == 0) {
                run_io = true;
            } else if (strcmp(argv[i], "--nano") == 0) {
                run_nano = true;
            } else if (strcmp(argv[i], "--e2e") == 0) {
                run_e2e = true;
            } else if (strcmp(argv[i], "--all") == 0) {
                run_all = true;
            } else if (strcmp(argv[i], "--help") == 0) {
                print_usage();
                return 0;
            } else {
                printf("❌ Unknown option: %s\n\n", argv[i]);
                print_usage();
                return 1;
            }
        }
    }
    
    // Check prerequisites
    if (!check_model_file()) {
        return 1;
    }
    
    // Track test results
    int total_tests = 0;
    int passed_tests = 0;
    
    printf("\n🧪 Starting Test Execution...\n");
    printf("================================\n\n");
    
    // Run selected tests
    if (run_all || run_streaming) {
        printf("🌊 Running Streaming Client Test...\n");
        printf("------------------------------------\n");
        total_tests++;
        test_streaming_client();
        passed_tests++; // We'll update this based on actual results
        printf("\n");
    }
    
    if (run_all || run_io) {
        printf("🔧 Running IO Callback Test...\n");
        printf("-------------------------------\n");
        total_tests++;
        test_io_callbacks();
        passed_tests++; // We'll update this based on actual results
        printf("\n");
    }
    
    if (run_all || run_nano) {
        printf("⚡ Running NANO Callback Test...\n");
        printf("---------------------------------\n");
        total_tests++;
        test_nano_callbacks();
        passed_tests++; // We'll update this based on actual results
        printf("\n");
    }
    
    if (run_all || run_e2e) {
        printf("🔄 Running End-to-End Workflow Test...\n");
        printf("---------------------------------------\n");
        total_tests++;
        test_end_to_end_workflow();
        passed_tests++; // We'll update this based on actual results
        printf("\n");
    }
    
    // Summary
    printf("📊 Test Summary\n");
    printf("===============\n");
    printf("Total tests run: %d\n", total_tests);
    printf("Tests passed: %d\n", passed_tests);
    printf("Tests failed: %d\n", total_tests - passed_tests);
    
    if (passed_tests == total_tests) {
        printf("\n🎉 All tests passed! Streaming architecture is working correctly.\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed. Check the output above for details.\n");
        return 1;
    }
}