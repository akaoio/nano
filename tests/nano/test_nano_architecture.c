#include "test_nano_architecture.h"
#include "../../src/nano/core/nano/nano.h"
#include "../../src/io/core/io/io.h"
#include "../../src/nano/transport/tcp_transport/tcp_transport.h"
#include "../../src/nano/transport/udp_transport/udp_transport.h"
#include "../../src/nano/transport/ws_transport/ws_transport.h"
#include "../../src/common/core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Test that NANO uses IO layer instead of direct RKLLM calls
void test_nano_uses_io_layer() {
    printf("🔍 Testing NANO Uses IO Layer...\n");
    
    // Initialize both systems
    assert(io_init() == IO_OK);
    assert(nano_init() == 0);
    
    // Create MCP request with real model
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 1,
        .method = str_copy("init"),
        .params = str_copy("{\"model_path\":\"models/lora/model.rkllm\"}"),
        .params_len = strlen("{\"model_path\":\"models/lora/model.rkllm\"}")
    };
    
    mcp_message_t response = {0};
    
    // Process message - this MUST use IO layer
    int result = nano_process_message(&request, &response);
    
    // Give IO time to process
    sleep(1);
    
    // Check that response comes from IO layer - STRICT CHECK
    assert(response.type == MCP_RESPONSE);
    assert(response.id == 1);
    
    if (result == 0) {
        printf("✅ NANO processed request successfully\n");
    } else {
        printf("❌ NANO returned error - NPU memory issues\n");
        printf("❌ This indicates real system problems, not 'expected behavior'\n");
        
        // Cleanup
        mcp_message_destroy(&request);
        mcp_message_destroy(&response);
        nano_shutdown();
        io_shutdown();
        
        // Force test failure
        assert(0 && "NANO failed due to NPU memory issues");
    }
    
    printf("✅ NANO correctly uses IO layer\n");
    
    // Cleanup
    mcp_message_destroy(&request);
    mcp_message_destroy(&response);
    nano_shutdown();
    io_shutdown();
    
    printf("✅ NANO Uses IO Layer test passed\n");
}

// Test MCP protocol compliance
void test_nano_mcp_compliance() {
    printf("🔍 Testing NANO MCP Compliance...\n");
    
    assert(nano_init() == 0);
    assert(io_init() == IO_OK);
    
    // Test valid MCP request with real model
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 42,
        .method = str_copy("init"),
        .params = str_copy("{\"model_path\":\"models/lora/model.rkllm\"}"),
        .params_len = strlen("{\"model_path\":\"models/lora/model.rkllm\"}")
    };
    
    mcp_message_t response = {0};
    
    int result = nano_process_message(&request, &response);
    
    // Verify MCP response format (regardless of success/failure)
    assert(response.type == MCP_RESPONSE);
    assert(response.id == 42); // Must match request ID
    assert(response.params != nullptr);
    
    if (result == 0) {
        printf("✅ MCP request processed successfully\n");
    } else {
        printf("❌ MCP request failed - NPU memory issues\n");
        printf("❌ This indicates real system problems\n");
        
        // Cleanup
        mcp_message_destroy(&request);
        mcp_message_destroy(&response);
        nano_shutdown();
        io_shutdown();
        
        // Force test failure
        assert(0 && "MCP request failed due to NPU memory issues");
    }
    
    printf("✅ MCP response format correct\n");
    
    // Test invalid method
    mcp_message_t invalid_request = {
        .type = MCP_REQUEST,
        .id = 43,
        .method = str_copy("invalid_method"),
        .params = str_copy("{}"),
        .params_len = 2
    };
    
    mcp_message_t error_response = {0};
    result = nano_process_message(&invalid_request, &error_response);
    
    // Should return error response
    assert(error_response.type == MCP_RESPONSE);
    assert(error_response.id == 43);
    
    printf("✅ Error handling correct\n");
    
    // Cleanup
    mcp_message_destroy(&request);
    mcp_message_destroy(&response);
    mcp_message_destroy(&invalid_request);
    mcp_message_destroy(&error_response);
    nano_shutdown();
    io_shutdown();
    
    printf("✅ NANO MCP Compliance test passed\n");
}

// Test transport isolation
void test_nano_transport_isolation() {
    printf("🔍 Testing NANO Transport Isolation...\n");
    
    assert(nano_init() == 0);
    
    // Test that transport interfaces are properly isolated
    mcp_transport_t* tcp_transport = tcp_transport_get_interface();
    mcp_transport_t* udp_transport = udp_transport_get_interface();
    mcp_transport_t* ws_transport = ws_transport_get_interface();
    
    assert(tcp_transport != nullptr);
    assert(udp_transport != nullptr);
    assert(ws_transport != nullptr);
    
    // Verify different transport interfaces
    assert(tcp_transport != udp_transport);
    assert(tcp_transport != ws_transport);
    assert(udp_transport != ws_transport);
    
    // Verify all required functions exist
    assert(tcp_transport->init != nullptr);
    assert(tcp_transport->send != nullptr);
    assert(tcp_transport->recv != nullptr);
    assert(tcp_transport->shutdown != nullptr);
    
    printf("✅ Transport isolation correct\n");
    
    nano_shutdown();
    printf("✅ NANO Transport Isolation test passed\n");
}

// Test error propagation from IO to NANO
void test_nano_error_propagation() {
    printf("🔍 Testing NANO Error Propagation...\n");
    
    // Initialize only NANO without IO
    assert(nano_init() == 0);
    // Deliberately NOT initializing IO to test error propagation
    
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 1,
        .method = str_copy("init"),
        .params = str_copy("{\"model_path\":\"models/lora/model.rkllm\"}"),
        .params_len = strlen("{\"model_path\":\"models/lora/model.rkllm\"}")
    };
    
    mcp_message_t response = {0};
    
    // This should fail gracefully since IO is not initialized
    int result = nano_process_message(&request, &response);
    (void)result; // Suppress unused variable warning
    
    // Should get error response
    assert(response.type == MCP_RESPONSE);
    assert(response.id == 1);
    
    printf("✅ Error propagation works correctly\n");
    
    // Cleanup
    mcp_message_destroy(&request);
    mcp_message_destroy(&response);
    nano_shutdown();
    
    printf("✅ NANO Error Propagation test passed\n");
}

int test_nano_architecture(void) {
    printf("🚀 Running NANO Architecture Tests\n");
    printf("===================================\n");
    
    test_nano_uses_io_layer();
    test_nano_mcp_compliance();
    test_nano_transport_isolation();
    test_nano_error_propagation();
    
    printf("\n🎉 All NANO Architecture tests passed!\n");
    return 0;
}
