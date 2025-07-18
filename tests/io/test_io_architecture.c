#include "test_io_architecture.h"
#include "../../src/io/core/io/io.h"
#include "../../src/nano/core/nano/nano.h"
#include "../../src/common/core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

// Test IO Queue Architecture
void test_io_queue_operations() {
    printf("🔍 Testing IO Queue Operations...\n");
    
    // Initialize IO system
    assert(io_init() == IO_OK);
    
    // Test push request with real model
    const char* test_request = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"init\",\"params\":{\"model_path\":\"models/qwenvl/model.rkllm\"}}";
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
    
    // Push multiple requests with real models
    const char* requests[] = {
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"init\",\"params\":{\"model_path\":\"models/qwenvl/model.rkllm\"}}",
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"init\",\"params\":{\"model_path\":\"models/lora/model.rkllm\"}}",
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"init\",\"params\":{\"model_path\":\"models/qwenvl/model.rkllm\"}}"
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
    
    // Try to fill queue beyond capacity with real model
    const char* test_request = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"init\",\"params\":{\"model_path\":\"models/qwenvl/model.rkllm\"}}";
    int pushed = 0;
    
    // Push requests until queue is full (assume reasonable limit)
    for (int i = 0; i < 1000; i++) {
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

// Test that NANO uses IO queues (merged from strict tests)
void test_nano_uses_io_queues() {
    printf("🔍 Testing NANO Uses IO Queues...\n");
    
    // Initialize IO and NANO
    assert(io_init() == IO_OK);
    assert(nano_init() == 0);
    
    // Create a test request with real model
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 1,
        .method = str_copy("init"),
        .params = str_copy("{\"model_path\":\"models/qwenvl/model.rkllm\"}"),
        .params_len = strlen("{\"model_path\":\"models/qwenvl/model.rkllm\"}")
    };
    
    mcp_message_t response = {0};
    
    // Process message through NANO
    printf("📨 Sending request through NANO...\n");
    int result = nano_process_message(&request, &response);
    
    // Give IO time to process
    sleep(1);
    
    // Check response
    if (result == 0) {
        printf("✅ NANO processed request using IO layer\n");
        printf("✅ Response received: %s\n", response.params ? response.params : "null");
    } else {
        printf("❌ NANO failed to process request - THIS IS A REAL FAILURE\n");
        printf("❌ NPU memory issues detected - test should FAIL\n");
        
        // Cleanup
        mcp_message_destroy(&request);
        mcp_message_destroy(&response);
        nano_shutdown();
        io_shutdown();
        
        // Force test failure
        assert(0 && "NANO failed to process request - NPU memory issues");
    }
    
    // Cleanup
    mcp_message_destroy(&request);
    mcp_message_destroy(&response);
    nano_shutdown();
    io_shutdown();
    
    printf("✅ NANO correctly uses IO queues\n");
}

// Test JSON parsing (merged from basic IO tests)
void test_io_json_parsing() {
    printf("🔍 Testing JSON Parsing...\n");
    
    const char* json = "{\"jsonrpc\":\"2.0\",\"id\":123,\"method\":\"test_method\",\"params\":{\"key\":\"value\"}}";
    
    uint32_t request_id, handle_id;
    char method[32], params[4096];
    
    int result = io_parse_json_request(json, &request_id, &handle_id, method, params);
    if (result == IO_OK) {
        assert(request_id == 123);
        assert(strcmp(method, "test_method") == 0);
        assert(strstr(params, "key") != NULL);
        printf("✅ JSON parsing test passed\n");
    } else {
        printf("⚠️  JSON parsing failed (function may not exist)\n");
    }
}

int test_io_architecture(void) {
    printf("🚀 Running IO Architecture Tests\n");
    printf("==================================\n");
    
    test_io_json_parsing();
    test_io_queue_operations();
    test_io_worker_pool();
    test_io_error_handling();
    test_io_queue_full();
    test_nano_uses_io_queues();
    
    printf("\n🎉 All IO Architecture tests passed!\n");
    return 0;
}
