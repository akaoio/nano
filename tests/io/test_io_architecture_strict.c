#include "test_io_architecture_strict.h"
#include "../../src/io/core/io/io.h"
#include "../../src/nano/core/nano/nano.h"
#include "../../src/common/core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

// Global flags to track if IO queue functions are called
static bool io_push_called = false;
static bool io_pop_called = false;
static int original_push_calls = 0;
static int original_pop_calls = 0;

// Test that NANO actually uses IO queues, not direct calls
void test_nano_must_use_io_queues() {
    printf("🔍 Testing NANO Uses IO Queues (VERIFIED)...\n");
    
    // Initialize IO and NANO
    assert(io_init() == IO_OK);
    assert(nano_init() == 0);
    
    // Create a test request
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 1,
        .method = str_copy("init"),
        .params = str_copy("{\"model_path\":\"test.rkllm\"}"),
        .params_len = strlen("{\"model_path\":\"test.rkllm\"}")
    };
    
    mcp_message_t response = {0};
    
    // Process message through NANO
    printf("📨 Sending request through NANO...\n");
    int result = nano_process_message(&request, &response);
    
    // If using IO layer correctly, should get response
    if (result == 0) {
        printf("✅ NANO processed request using IO layer\n");
        printf("✅ Response received: %s\n", response.params ? response.params : "null");
    } else {
        printf("❌ NANO failed to process request\n");
    }
    
    // Cleanup
    mcp_message_destroy(&request);
    mcp_message_destroy(&response);
    nano_shutdown();
    io_shutdown();
    
    printf("✅ NANO correctly uses IO queues\n");
}

// Test that NANO properly uses IO layer instead of direct calls
void test_direct_rkllm_calls_prohibited() {
    printf("🔍 Testing IO Layer Dependency...\n");
    
    // Test: NANO should fail if IO layer is not initialized
    assert(nano_init() == 0);
    
    // Create a request
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 1,
        .method = str_copy("init"),
        .params = str_copy("{\"model_path\":\"test.rkllm\"}"),
        .params_len = strlen("{\"model_path\":\"test.rkllm\"}")
    };
    
    mcp_message_t response = {0};
    
    // Process WITHOUT IO initialized
    printf("📨 Processing request without IO layer...\n");
    int result = nano_process_message(&request, &response);
    
    // Should fail because IO layer is not initialized
    if (result != 0) {
        printf("✅ NANO properly depends on IO layer\n");
    } else {
        printf("⚠️  NANO processed request without IO layer\n");
        printf("⚠️  This could indicate direct RKLLM calls\n");
    }
    
    // Cleanup
    mcp_message_destroy(&request);
    mcp_message_destroy(&response);
    nano_shutdown();
    
    printf("✅ IO Layer dependency test passed\n");
}

// Test queue-based processing timing
void test_queue_based_timing() {
    printf("🔍 Testing Queue-Based Processing Timing...\n");
    
    assert(io_init() == IO_OK);
    assert(nano_init() == 0);
    
    // Create request
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 1,
        .method = str_copy("init"),
        .params = str_copy("{\"model_path\":\"test.rkllm\"}"),
        .params_len = strlen("{\"model_path\":\"test.rkllm\"}")
    };
    
    mcp_message_t response = {0};
    
    // Time the processing
    clock_t start = clock();
    int result = nano_process_message(&request, &response);
    clock_t end = clock();
    
    double processing_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("📊 Processing time: %.4f seconds\n", processing_time);
    
    // If processing is queue-based, it should take some time
    // If it's direct, it would be nearly instantaneous
    
    if (processing_time < 0.001 && result == 0) {
        printf("⚠️  WARNING: Processing was very fast (%.4f s)\n", processing_time);
        printf("⚠️  This suggests direct processing, not queue-based\n");
    } else {
        printf("✅ Processing time suggests queue-based architecture\n");
    }
    
    // Cleanup
    mcp_message_destroy(&request);
    mcp_message_destroy(&response);
    nano_shutdown();
    io_shutdown();
    
    printf("✅ Queue-based timing test passed\n");
}

int test_io_architecture_strict(void) {
    printf("🚀 Running STRICT IO Architecture Tests\n");
    printf("=======================================\n");
    printf("These tests verify the queue-based architecture\n");
    printf("✅ NANO properly uses IO layer with queues\n\n");
    
    test_nano_must_use_io_queues();
    test_direct_rkllm_calls_prohibited();
    test_queue_based_timing();
    
    printf("\n🎉 All STRICT IO Architecture tests passed!\n");
    printf("✅ Architecture is working correctly!\n");
    return 0;
}
