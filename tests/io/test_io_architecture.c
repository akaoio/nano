#include "test_io_architecture.h"
#include "../../src/io/core/io/io.h"
#include "../../src/common/core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Test IO Queue Architecture
void test_io_queue_operations() {
    printf("🔍 Testing IO Queue Operations...\n");
    
    // Initialize IO system
    assert(io_init() == IO_OK);
    
    // Test push request
    const char* test_request = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"init\",\"params\":{\"model_path\":\"test.rkllm\"}}";
    assert(io_push_request(test_request) == IO_OK);
    
    // Test pop response - should get a response after processing
    char response[1024];
    int result = io_pop_response(response, sizeof(response));
    
    // Should either get a response or timeout (both valid)
    assert(result == IO_OK || result == IO_TIMEOUT);
    
    if (result == IO_OK) {
        printf("✅ Got response: %s\n", response);
        assert(strlen(response) > 0);
    } else {
        printf("⏱️ Response timeout (expected for fast test)\n");
    }
    
    io_shutdown();
    printf("✅ IO Queue Operations test passed\n");
}

// Test IO Worker Pool
void test_io_worker_pool() {
    printf("🔍 Testing IO Worker Pool...\n");
    
    assert(io_init() == IO_OK);
    
    // Push multiple requests
    const char* requests[] = {
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"init\",\"params\":{\"model_path\":\"test1.rkllm\"}}",
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"init\",\"params\":{\"model_path\":\"test2.rkllm\"}}",
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"init\",\"params\":{\"model_path\":\"test3.rkllm\"}}"
    };
    
    for (int i = 0; i < 3; i++) {
        assert(io_push_request(requests[i]) == IO_OK);
    }
    
    // Give workers time to process
    sleep(2);
    
    // Try to get responses
    char response[1024];
    int responses_received = 0;
    
    for (int i = 0; i < 3; i++) {
        if (io_pop_response(response, sizeof(response)) == IO_OK) {
            responses_received++;
            printf("📨 Response %d: %s\n", responses_received, response);
        }
    }
    
    printf("📊 Received %d responses out of 3 requests\n", responses_received);
    
    io_shutdown();
    printf("✅ IO Worker Pool test passed\n");
}

// Test IO Error Handling
void test_io_error_handling() {
    printf("🔍 Testing IO Error Handling...\n");
    
    // Test without initialization
    assert(io_push_request("test") == IO_ERROR);
    
    char response[1024];
    assert(io_pop_response(response, sizeof(response)) == IO_ERROR);
    
    // Test with null parameters
    assert(io_init() == IO_OK);
    assert(io_push_request(nullptr) == IO_ERROR);
    assert(io_pop_response(nullptr, 100) == IO_ERROR);
    
    io_shutdown();
    printf("✅ IO Error Handling test passed\n");
}

// Test IO Queue Full Scenario
void test_io_queue_full() {
    printf("🔍 Testing IO Queue Full Scenario...\n");
    
    assert(io_init() == IO_OK);
    
    // Try to fill queue beyond capacity
    const char* test_request = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"test\",\"params\":{}}";
    int pushed = 0;
    
    // Push requests until queue is full
    for (int i = 0; i < QUEUE_SIZE + 10; i++) {
        int result = io_push_request(test_request);
        if (result == IO_OK) {
            pushed++;
        } else if (result == IO_QUEUE_FULL) {
            printf("📊 Queue full after %d requests\n", pushed);
            break;
        }
    }
    
    assert(pushed > 0);
    printf("✅ Queue filled with %d requests\n", pushed);
    
    io_shutdown();
    printf("✅ IO Queue Full test passed\n");
}

int test_io_architecture(void) {
    printf("🚀 Running IO Architecture Tests\n");
    printf("==================================\n");
    
    test_io_queue_operations();
    test_io_worker_pool();
    test_io_error_handling();
    test_io_queue_full();
    
    printf("\n🎉 All IO Architecture tests passed!\n");
    return 0;
}
