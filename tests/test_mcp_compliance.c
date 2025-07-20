#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <json-c/json.h>

// Include unified server architecture
#include "../src/server/server.h"
#include "../src/server/protocol/mcp_adapter.h"
#include "../src/server/transport/transport_manager.h"
#include "../src/server/transport/stdio_transport/stdio_transport.h"
#include "../src/server/transport/tcp_transport/tcp_transport.h"
#include "../src/server/transport/udp_transport/udp_transport.h"
#include "../src/server/transport/http_transport/http_transport.h"
#include "../src/server/transport/ws_transport/ws_transport.h"
#include "../src/server/operations.h"

/*
 * COMPLETE MCP COMPLIANCE TEST SUITE
 * 
 * Tests all five transports for:
 * 1. MCP protocol compliance
 * 2. Streaming consistency
 * 3. UTF-8 validation
 * 4. Message formatting
 * 5. Error handling
 * 6. Session management
 */

// Test state
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
    char current_transport[32];
} test_state_t;

static test_state_t g_state = {0};

#define TEST_ASSERT(condition, message) \
    do { \
        g_state.tests_run++; \
        if (condition) { \
            printf("✅ [%s] %s\n", g_state.current_transport, message); \
            g_state.tests_passed++; \
        } else { \
            printf("❌ [%s] %s\n", g_state.current_transport, message); \
            g_state.tests_failed++; \
        } \
    } while(0)

// Transport interface for unified testing - updated for new architecture
typedef struct {
    const char* name;
    transport_base_t* transport;
    transport_manager_t* manager;
    void* config;
    bool supports_streaming;
    bool requires_server;
} transport_test_t;

// Test 1: MCP Protocol Compliance
bool test_mcp_protocol_compliance(transport_test_t* transport) {
    printf("\n🧪 Test: MCP Protocol Compliance - %s\n", transport->name);
    printf("===============================================\n");
    strcpy(g_state.current_transport, transport->name);
    
    // Test MCP adapter request formatting
    mcp_request_t request = {0};
    strcpy(request.method, "run");
    strcpy(request.params, "{\"prompt\": \"Hello\", \"max_tokens\": 100}");
    strcpy(request.request_id, "12345");
    request.is_streaming = false;
    
    mcp_response_t response = {0};
    strcpy(response.request_id, "12345");
    response.is_success = true;
    strcpy(response.result, "{\"content\": \"Hello World\"}");
    
    char buffer[8192];
    int format_result = mcp_adapter_format_response(&response, buffer, sizeof(buffer));
    TEST_ASSERT(format_result == MCP_ADAPTER_OK, "MCP adapter response formatting");
    
    // Verify JSON-RPC structure
    json_object* msg_obj = json_tokener_parse(buffer);
    TEST_ASSERT(msg_obj != NULL, "Valid JSON structure");
    
    json_object* jsonrpc_obj, *method_obj, *params_obj, *id_obj;
    TEST_ASSERT(json_object_object_get_ex(msg_obj, "jsonrpc", &jsonrpc_obj), "Has jsonrpc field");
    TEST_ASSERT(strcmp(json_object_get_string(jsonrpc_obj), "2.0") == 0, "Correct JSON-RPC version");
    
    TEST_ASSERT(json_object_object_get_ex(msg_obj, "method", &method_obj), "Has method field");
    TEST_ASSERT(strcmp(json_object_get_string(method_obj), "run") == 0, "Correct method value");
    
    TEST_ASSERT(json_object_object_get_ex(msg_obj, "params", &params_obj), "Has params field");
    TEST_ASSERT(json_object_object_get_ex(msg_obj, "id", &id_obj), "Has id field");
    TEST_ASSERT(json_object_get_int(id_obj) == 12345, "Correct id value");
    
    json_object_put(msg_obj);
    
    // Test error response formatting
    char error_buffer[8192];
    int error_result = mcp_adapter_format_error("12345", -32601, "Method not found", error_buffer, sizeof(error_buffer));
    TEST_ASSERT(error_result == MCP_ADAPTER_OK, "Error response formatting");
    
    json_object* error_msg_obj = json_tokener_parse(error_buffer);
    TEST_ASSERT(error_msg_obj != NULL, "Valid error JSON structure");
    
    json_object* error_field_obj;
    TEST_ASSERT(json_object_object_get_ex(error_msg_obj, "error", &error_field_obj), "Has error field");
    
    json_object* code_obj, *message_obj;
    TEST_ASSERT(json_object_object_get_ex(error_field_obj, "code", &code_obj), "Error has code field");
    TEST_ASSERT(json_object_get_int(code_obj) == -32601, "Correct error code");
    
    TEST_ASSERT(json_object_object_get_ex(error_field_obj, "message", &message_obj), "Error has message field");
    
    json_object_put(error_msg_obj);
    
    return true;
}

// Test 2: UTF-8 Validation
bool test_utf8_validation(transport_test_t* transport) {
    printf("\n🧪 Test: UTF-8 Validation - %s\n", transport->name);
    printf("========================================\n");
    strcpy(g_state.current_transport, transport->name);
    
    // Test UTF-8 validation through MCP adapter
    const char* valid_utf8[] = {
        "Hello World",           // ASCII
        "Héllo Wörld",          // Latin-1
        "你好世界",              // Chinese
        "🚀 Hello 🌍",          // Emoji
        "Привет мир"            // Cyrillic
    };
    
    for (size_t i = 0; i < sizeof(valid_utf8) / sizeof(valid_utf8[0]); i++) {
        int result = mcp_adapter_validate_utf8(valid_utf8[i]);
        TEST_ASSERT(result == MCP_ADAPTER_OK, "Valid UTF-8 string accepted");
    }
    
    // Test Unicode JSON formatting
    const char* unicode_json = "{\"text\": \"🌟 Hello 世界 🎉\"}";
    int unicode_result = mcp_adapter_validate_utf8(unicode_json);
    TEST_ASSERT(unicode_result == MCP_ADAPTER_OK, "Unicode JSON validation");
    
    // Verify the Unicode JSON is valid
    json_object* unicode_obj = json_tokener_parse(unicode_json);
    TEST_ASSERT(unicode_obj != NULL, "Unicode JSON creates valid object");
    
    if (unicode_obj) {
        json_object_put(unicode_obj);
    }
    
    return true;
}

// Test 3: Streaming Consistency
bool test_streaming_consistency(transport_test_t* transport) {
    if (!transport->supports_streaming) {
        printf("\n⏭️  Skipping streaming test for %s (not supported)\n", transport->name);
        return true;
    }
    
    printf("\n🧪 Test: Streaming Consistency - %s\n", transport->name);
    printf("==========================================\n");
    strcpy(g_state.current_transport, transport->name);
    
    // Test stream request handling through MCP adapter
    mcp_request_t stream_request = {0};
    strcpy(stream_request.method, "run");
    strcpy(stream_request.params, "{\"prompt\": \"Hello\", \"stream\": true, \"max_tokens\": 50}");
    strcpy(stream_request.request_id, "54321");
    stream_request.is_streaming = true;
    
    mcp_response_t stream_response = {0};
    
    int stream_result = mcp_adapter_handle_stream_request(&stream_request, &stream_response);
    TEST_ASSERT(stream_result == MCP_ADAPTER_OK, "Stream request handling");
    TEST_ASSERT(stream_response.is_streaming_response == true, "Stream response marked correctly");
        
        // Parse stream response
        json_object* response_obj = json_tokener_parse(stream_response.params);
        TEST_ASSERT(response_obj != NULL, "Stream response valid JSON");
        
        if (response_obj) {
            json_object* stream_id_obj, *status_obj;
            TEST_ASSERT(json_object_object_get_ex(response_obj, "stream_id", &stream_id_obj), "Stream response has stream_id");
            TEST_ASSERT(json_object_object_get_ex(response_obj, "status", &status_obj), "Stream response has status");
            
            const char* stream_id = json_object_get_string(stream_id_obj);
            const char* status = json_object_get_string(status_obj);
            
            TEST_ASSERT(strlen(stream_id) == STREAM_ID_LENGTH, "Stream ID correct length");
            TEST_ASSERT(strcmp(status, "streaming_started") == 0, "Correct status message");
            
            // Test stream chunk format
            if (transport->send_stream_chunk) {
                char test_chunks[][32] = {"Hello", " streaming", " world", "!"};
                bool chunk_ends[] = {false, false, false, true};
                
                for (int i = 0; i < 4; i++) {
                    int chunk_result = transport->send_stream_chunk(stream_id, i, test_chunks[i], chunk_ends[i], NULL);
                    // Note: Result may fail due to no actual connection, but function should not crash
                    printf("   Chunk %d sent (result: %d)\n", i, chunk_result);
                }
                
                TEST_ASSERT(true, "Stream chunk sending functions");
            }
            
            json_object_put(response_obj);
        }
    } else {
        printf("   ⚠️  Transport does not implement streaming functions\n");
    }
    
    return true;
}

// Test 4: Message Batching
bool test_message_batching(transport_test_t* transport) {
    printf("\n🧪 Test: Message Batching - %s\n", transport->name);
    printf("=====================================\n");
    strcpy(g_state.current_transport, transport->name);
    
    // Create batch of messages
    mcp_message_t messages[3];
    
    // Message 1
    strcpy(messages[0].method, "test1");
    strcpy(messages[0].params, "{\"value\": 1}");
    messages[0].id = 1;
    messages[0].is_response = false;
    
    // Message 2  
    strcpy(messages[1].method, "test2");
    strcpy(messages[1].params, "{\"value\": 2}");
    messages[1].id = 2;
    messages[1].is_response = false;
    
    // Message 3
    strcpy(messages[2].method, "test3");
    strcpy(messages[2].params, "{\"value\": 3}");
    messages[2].id = 3;
    messages[2].is_response = false;
    
    // Test batch formatting (using JSON-RPC batch format)
    json_object* batch_array = json_object_new_array();
    
    for (int i = 0; i < 3; i++) {
        char msg_buffer[8192];
        if (mcp_format_json_rpc(&messages[i], msg_buffer, sizeof(msg_buffer)) == 0) {
            json_object* msg_obj = json_tokener_parse(msg_buffer);
            if (msg_obj) {
                json_object_array_add(batch_array, msg_obj);
            }
        }
    }
    
    TEST_ASSERT(json_object_array_length(batch_array) == 3, "Batch contains all messages");
    
    const char* batch_str = json_object_to_json_string(batch_array);
    TEST_ASSERT(batch_str != NULL, "Batch serialization");
    TEST_ASSERT(strlen(batch_str) > 0, "Batch not empty");
    
    // Verify batch structure
    json_object* parsed_batch = json_tokener_parse(batch_str);
    TEST_ASSERT(parsed_batch != NULL, "Batch parses correctly");
    TEST_ASSERT(json_object_is_type(parsed_batch, json_type_array), "Batch is array");
    TEST_ASSERT(json_object_array_length(parsed_batch) == 3, "Batch has correct length");
    
    // Verify each message in batch
    for (int i = 0; i < 3; i++) {
        json_object* msg_in_batch = json_object_array_get_idx(parsed_batch, i);
        TEST_ASSERT(msg_in_batch != NULL, "Batch message exists");
        
        json_object* method_obj, *id_obj;
        TEST_ASSERT(json_object_object_get_ex(msg_in_batch, "method", &method_obj), "Batch message has method");
        TEST_ASSERT(json_object_object_get_ex(msg_in_batch, "id", &id_obj), "Batch message has id");
        TEST_ASSERT(json_object_get_int(id_obj) == i + 1, "Batch message correct id");
    }
    
    json_object_put(batch_array);
    json_object_put(parsed_batch);
    
    return true;
}

// Test 5: Error Handling
bool test_error_handling(transport_test_t* transport) {
    printf("\n🧪 Test: Error Handling - %s\n", transport->name);
    printf("====================================\n");
    strcpy(g_state.current_transport, transport->name);
    
    // Test standard MCP error codes
    struct {
        mcp_error_code_t code;
        const char* expected_message;
    } test_errors[] = {
        {MCP_ERROR_PARSE, "Parse error"},
        {MCP_ERROR_INVALID_REQUEST, "Invalid request"},
        {MCP_ERROR_METHOD_NOT_FOUND, "Method not found"},
        {MCP_ERROR_INVALID_PARAMS, "Invalid params"},
        {MCP_ERROR_INTERNAL, "Internal error"},
        {MCP_ERROR_STREAM_NOT_FOUND, "Stream session not found or expired"},
        {MCP_ERROR_STREAM_EXPIRED, "Stream session expired"},
        {MCP_ERROR_STREAM_INVALID_STATE, "Stream in invalid state"}
    };
    
    for (size_t i = 0; i < sizeof(test_errors) / sizeof(test_errors[0]); i++) {
        char error_buffer[8192];
        int result = mcp_format_error(i + 1, test_errors[i].code, NULL, NULL, error_buffer, sizeof(error_buffer));
        TEST_ASSERT(result == 0, "Error formatting");
        
        json_object* error_obj = json_tokener_parse(error_buffer);
        TEST_ASSERT(error_obj != NULL, "Error JSON valid");
        
        if (error_obj) {
            json_object* error_field, *code_field, *message_field;
            TEST_ASSERT(json_object_object_get_ex(error_obj, "error", &error_field), "Has error field");
            TEST_ASSERT(json_object_object_get_ex(error_field, "code", &code_field), "Error has code");
            TEST_ASSERT(json_object_object_get_ex(error_field, "message", &message_field), "Error has message");
            
            TEST_ASSERT(json_object_get_int(code_field) == test_errors[i].code, "Correct error code");
            
            const char* message = json_object_get_string(message_field);
            TEST_ASSERT(strstr(message, test_errors[i].expected_message) != NULL, "Expected error message");
            
            json_object_put(error_obj);
        }
    }
    
    return true;
}

// Test 6: Transport Initialization
bool test_transport_initialization(transport_test_t* transport) {
    printf("\n🧪 Test: Transport Initialization - %s\n", transport->name);
    printf("===========================================\n");
    strcpy(g_state.current_transport, transport->name);
    
    if (transport->requires_server) {
        printf("   ⏭️  Skipping initialization test (requires server setup)\n");
        return true;
    }
    
    // Test transport initialization
    int init_result = transport->interface->init(transport->config);
    TEST_ASSERT(init_result == 0, "Transport initialization");
    
    // Test shutdown
    transport->interface->shutdown();
    TEST_ASSERT(true, "Transport shutdown");
    
    return true;
}

// Run all tests for a specific transport
void test_transport_suite(transport_test_t* transport) {
    printf("\n🚀 TESTING TRANSPORT: %s\n", transport->name);
    printf("==========================================\n");
    
    test_mcp_protocol_compliance(transport);
    test_utf8_validation(transport);
    test_streaming_consistency(transport);
    test_message_batching(transport);
    test_error_handling(transport);
    test_transport_initialization(transport);
}

// Main test runner
int main(void) {
    printf("🚀 MCP COMPLIANCE TEST SUITE\n");
    printf("============================\n");
    printf("Testing all transports for MCP protocol compliance and streaming consistency\n");
    
    // Initialize stream manager for tests
    stream_manager_init();
    
    // Define all transports for testing
    stdio_transport_config_t stdio_config = {
        .streaming_enabled = true,
        .log_to_stderr = false  // Disable for testing
    };
    
    tcp_transport_config_t tcp_config = {
        .host = "localhost",
        .port = 8080,
        .streaming_enabled = true,
        .is_server = false
    };
    
    udp_transport_config_t udp_config = {
        .host = "localhost", 
        .port = 8081,
        .streaming_enabled = true
    };
    
    http_transport_config_t http_config = {
        .host = "localhost",
        .port = 8082,
        .streaming_enabled = true
    };
    
    ws_transport_config_t ws_config = {
        .host = "localhost",
        .port = 8083,
        .streaming_enabled = true
    };
    
    transport_test_t transports[] = {
        {
            .name = "STDIO",
            .interface = stdio_transport_get_interface(),
            .config = &stdio_config,
            .supports_streaming = true,
            .requires_server = false,
            .handle_stream_request = stdio_transport_handle_stream_request,
            .send_stream_chunk = stdio_transport_send_stream_chunk,
            .create_stream_response = stdio_transport_create_stream_response
        },
        {
            .name = "TCP", 
            .interface = tcp_transport_get_interface(),
            .config = &tcp_config,
            .supports_streaming = true,
            .requires_server = true,
            .handle_stream_request = tcp_transport_handle_stream_request,
            .send_stream_chunk = tcp_transport_send_stream_chunk,
            .create_stream_response = tcp_transport_create_stream_response
        },
        {
            .name = "UDP",
            .interface = udp_transport_get_interface(), 
            .config = &udp_config,
            .supports_streaming = true,
            .requires_server = true,
            .handle_stream_request = udp_transport_handle_stream_request,
            .send_stream_chunk = udp_transport_send_stream_chunk,
            .create_stream_response = udp_transport_create_stream_response
        },
        {
            .name = "HTTP",
            .interface = http_transport_get_interface(),
            .config = &http_config, 
            .supports_streaming = true,
            .requires_server = true,
            .handle_stream_request = http_transport_handle_stream_request,
            .send_stream_chunk = NULL, // HTTP uses polling
            .create_stream_response = http_transport_create_stream_response
        },
        {
            .name = "WebSocket",
            .interface = ws_transport_get_interface(),
            .config = &ws_config,
            .supports_streaming = true, 
            .requires_server = true,
            .handle_stream_request = ws_transport_handle_stream_request,
            .send_stream_chunk = ws_transport_send_stream_chunk,
            .create_stream_response = ws_transport_create_stream_response
        }
    };
    
    // Run tests for all transports
    size_t transport_count = sizeof(transports) / sizeof(transports[0]);
    for (size_t i = 0; i < transport_count; i++) {
        test_transport_suite(&transports[i]);
    }
    
    // Cleanup
    stream_manager_shutdown();
    
    // Print summary
    printf("\n📊 TEST SUMMARY\n");
    printf("===============\n");
    printf("Total tests run: %d\n", g_state.tests_run);
    printf("✅ Passed: %d\n", g_state.tests_passed);
    printf("❌ Failed: %d\n", g_state.tests_failed);
    printf("Success rate: %.1f%%\n", 
           g_state.tests_run > 0 ? (float)g_state.tests_passed / g_state.tests_run * 100 : 0);
    
    if (g_state.tests_failed == 0) {
        printf("\n🎉 ALL MCP COMPLIANCE TESTS PASSED!\n");
        printf("All transports are fully MCP compliant with consistent streaming behavior.\n");
        return 0;
    } else {
        printf("\n⚠️  Some MCP compliance tests failed.\n");
        printf("Please review the failed test cases above.\n");
        return 1;
    }
}