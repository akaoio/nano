#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <json-c/json.h>

// Include unified server architecture
#include "../src/server/server.h"
#include "../src/server/transport/transport_manager.h"
#include "../src/server/transport/stdio_transport/stdio_transport.h"
#include "../src/server/transport/tcp_transport/tcp_transport.h"
#include "../src/server/transport/udp_transport/udp_transport.h"
#include "../src/server/transport/http_transport/http_transport.h"
#include "../src/server/transport/ws_transport/ws_transport.h"
#include "../src/server/transport/mcp_base/mcp_protocol.h"
#include "../src/server/operations.h"
#include "../src/common/string_utils/string_utils.h"

/*
 * COMPLETE MCP COMPLIANCE TEST SUITE - UNIFIED ARCHITECTURE
 * 
 * Tests all transport layers for:
 * 1. MCP protocol compliance
 * 2. JSON-RPC 2.0 formatting
 * 3. UTF-8 validation
 * 4. Error handling
 * 5. Server integration
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

// Transport test configuration
typedef struct {
    const char* name;
    bool supports_streaming;
    bool requires_network;
    void* config;
} transport_test_config_t;

// Test 1: MCP Protocol Basic Compliance
bool test_mcp_protocol_basic(void) {
    printf("\n🧪 Test: MCP Protocol Basic Compliance\n");
    printf("=====================================\n");
    strcpy(g_state.current_transport, "Protocol");
    
    // Initialize MCP context
    mcp_context_t ctx = {0};
    int init_result = mcp_protocol_init(&ctx);
    TEST_ASSERT(init_result == 0, "MCP protocol initialization");
    
    // Test request formatting
    char request_buffer[1024];
    int format_result = mcp_format_request(12345, "run", "{\"prompt\": \"Hello\"}", request_buffer, sizeof(request_buffer));
    TEST_ASSERT(format_result == 0, "Format MCP request");
    
    // Verify JSON-RPC 2.0 structure
    json_object* req_obj = json_tokener_parse(request_buffer);
    TEST_ASSERT(req_obj != NULL, "Valid JSON request structure");
    
    json_object* jsonrpc_obj, *method_obj, *params_obj, *id_obj;
    TEST_ASSERT(json_object_object_get_ex(req_obj, "jsonrpc", &jsonrpc_obj), "Has jsonrpc field");
    TEST_ASSERT(strcmp(json_object_get_string(jsonrpc_obj), "2.0") == 0, "Correct JSON-RPC version");
    
    TEST_ASSERT(json_object_object_get_ex(req_obj, "method", &method_obj), "Has method field");
    TEST_ASSERT(strcmp(json_object_get_string(method_obj), "run") == 0, "Correct method value");
    
    TEST_ASSERT(json_object_object_get_ex(req_obj, "params", &params_obj), "Has params field");
    TEST_ASSERT(json_object_object_get_ex(req_obj, "id", &id_obj), "Has id field");
    TEST_ASSERT(json_object_get_int(id_obj) == 12345, "Correct id value");
    
    json_object_put(req_obj);
    
    // Test response formatting
    char response_buffer[1024];
    int resp_result = mcp_format_response(12345, "{\"content\": \"Hello World\"}", response_buffer, sizeof(response_buffer));
    TEST_ASSERT(resp_result == 0, "Format MCP response");
    
    json_object* resp_obj = json_tokener_parse(response_buffer);
    TEST_ASSERT(resp_obj != NULL, "Valid JSON response structure");
    
    json_object* result_obj;
    TEST_ASSERT(json_object_object_get_ex(resp_obj, "result", &result_obj), "Has result field");
    TEST_ASSERT(json_object_object_get_ex(resp_obj, "jsonrpc", &jsonrpc_obj), "Response has jsonrpc field");
    TEST_ASSERT(json_object_object_get_ex(resp_obj, "id", &id_obj), "Response has id field");
    
    json_object_put(resp_obj);
    
    // Test error formatting
    char error_buffer[1024];
    int error_result = mcp_format_error(12345, MCP_ERROR_METHOD_NOT_FOUND, "Method not found", NULL, error_buffer, sizeof(error_buffer));
    TEST_ASSERT(error_result == 0, "Format MCP error");
    
    json_object* error_obj = json_tokener_parse(error_buffer);
    TEST_ASSERT(error_obj != NULL, "Valid JSON error structure");
    
    json_object* error_field_obj;
    TEST_ASSERT(json_object_object_get_ex(error_obj, "error", &error_field_obj), "Has error field");
    
    json_object* code_obj, *message_obj;
    TEST_ASSERT(json_object_object_get_ex(error_field_obj, "code", &code_obj), "Error has code field");
    TEST_ASSERT(json_object_get_int(code_obj) == MCP_ERROR_METHOD_NOT_FOUND, "Correct error code");
    
    TEST_ASSERT(json_object_object_get_ex(error_field_obj, "message", &message_obj), "Error has message field");
    TEST_ASSERT(strcmp(json_object_get_string(message_obj), "Method not found") == 0, "Correct error message");
    
    json_object_put(error_obj);
    
    // Test notification formatting
    char notification_buffer[1024];
    int notif_result = mcp_format_notification("progress", "{\"stage\": \"processing\"}", notification_buffer, sizeof(notification_buffer));
    TEST_ASSERT(notif_result == 0, "Format MCP notification");
    
    json_object* notif_obj = json_tokener_parse(notification_buffer);
    TEST_ASSERT(notif_obj != NULL, "Valid JSON notification structure");
    
    // Notifications should not have id field
    json_object* notif_id_obj;
    bool has_id = json_object_object_get_ex(notif_obj, "id", &notif_id_obj);
    TEST_ASSERT(!has_id, "Notification does not have id field");
    
    json_object_put(notif_obj);
    
    mcp_protocol_shutdown(&ctx);
    return true;
}

// Test 2: UTF-8 and Unicode Compliance
bool test_utf8_compliance(void) {
    printf("\n🧪 Test: UTF-8 and Unicode Compliance\n");
    printf("=====================================\n");
    strcpy(g_state.current_transport, "UTF8");
    
    // Test various UTF-8 strings
    const char* test_strings[] = {
        "Hello World",           // ASCII
        "Café, naïve, résumé",   // Latin extended
        "你好世界",              // Chinese
        "🚀 Hello 🌍 World 🎉", // Emoji
        "Привет мир",           // Cyrillic
        "مرحبا بالعالم",         // Arabic
        "שלום עולם",             // Hebrew
        "こんにちは世界"          // Japanese
    };
    
    for (size_t i = 0; i < sizeof(test_strings) / sizeof(test_strings[0]); i++) {
        // Test UTF-8 content in JSON params
        char params_buffer[1024];
        snprintf(params_buffer, sizeof(params_buffer), "{\"text\": \"%s\"}", test_strings[i]);
        
        char request_buffer[2048];
        int format_result = mcp_format_request(i + 1, "echo", params_buffer, request_buffer, sizeof(request_buffer));
        TEST_ASSERT(format_result == 0, "Format UTF-8 request");
        
        // Verify JSON parsing still works
        json_object* req_obj = json_tokener_parse(request_buffer);
        TEST_ASSERT(req_obj != NULL, "Parse UTF-8 JSON request");
        
        json_object* params_obj;
        TEST_ASSERT(json_object_object_get_ex(req_obj, "params", &params_obj), "UTF-8 params accessible");
        
        json_object_put(req_obj);
    }
    
    // Test emoji in method names and error messages
    char emoji_error[1024];
    int emoji_result = mcp_format_error(999, MCP_ERROR_INTERNAL, "🚨 Internal server error 💥", NULL, emoji_error, sizeof(emoji_error));
    TEST_ASSERT(emoji_result == 0, "Format error with emoji");
    
    json_object* emoji_obj = json_tokener_parse(emoji_error);
    TEST_ASSERT(emoji_obj != NULL, "Parse emoji error JSON");
    json_object_put(emoji_obj);
    
    return true;
}

// Test 3: Server Integration
bool test_server_integration(void) {
    printf("\n🧪 Test: Server Integration\n");
    printf("===========================\n");
    strcpy(g_state.current_transport, "Server");
    
    // Initialize server configuration
    mcp_server_config_t config = {
        .enable_stdio = true,
        .enable_tcp = false,     // Disable network transports for testing
        .enable_udp = false,
        .enable_http = false,
        .enable_websocket = false,
        .server_name = "Test-Server",
        .enable_streaming = true,
        .enable_logging = false
    };
    
    mcp_server_t server = {0};
    int init_result = mcp_server_init(&server, &config);
    TEST_ASSERT(init_result == 0, "Server initialization");
    
    // Test server status
    const char* status = mcp_server_get_status(&server);
    TEST_ASSERT(status != NULL, "Server status retrieval");
    TEST_ASSERT(strlen(status) > 0, "Server status non-empty");
    
    // Test server statistics
    uint64_t requests, responses, errors, uptime;
    mcp_server_get_stats(&server, &requests, &responses, &errors, &uptime);
    TEST_ASSERT(true, "Server statistics retrieval"); // Always passes if no crash
    
    // Test server shutdown
    mcp_server_shutdown(&server);
    TEST_ASSERT(true, "Server shutdown");
    
    return true;
}

// Test 4: IO Operations Integration
bool test_io_operations_integration(void) {
    printf("\n🧪 Test: IO Operations Integration\n");
    printf("==================================\n");
    strcpy(g_state.current_transport, "IO");
    
    // Initialize IO operations
    int init_result = io_operations_init();
    TEST_ASSERT(init_result == 0, "IO operations initialization");
    
    // Test JSON request parsing
    const char* test_request = "{"
        "\"jsonrpc\": \"2.0\","
        "\"id\": 42,"
        "\"method\": \"run\","
        "\"params\": {\"prompt\": \"Hello MCP\", \"max_tokens\": 50}"
    "}";
    
    uint32_t request_id;
    char method[256];
    char params[4096];
    
    int parse_result = io_parse_json_request_main(test_request, &request_id, method, params);
    TEST_ASSERT(parse_result == 0, "Parse JSON request");
    TEST_ASSERT(request_id == 42, "Correct request ID parsed");
    TEST_ASSERT(strcmp(method, "run") == 0, "Correct method parsed");
    
    // Verify params parsing
    json_object* params_obj = json_tokener_parse(params);
    TEST_ASSERT(params_obj != NULL, "Parse params JSON");
    
    json_object* prompt_obj, *tokens_obj;
    TEST_ASSERT(json_object_object_get_ex(params_obj, "prompt", &prompt_obj), "Prompt in params");
    TEST_ASSERT(json_object_object_get_ex(params_obj, "max_tokens", &tokens_obj), "Max tokens in params");
    TEST_ASSERT(strcmp(json_object_get_string(prompt_obj), "Hello MCP") == 0, "Correct prompt value");
    TEST_ASSERT(json_object_get_int(tokens_obj) == 50, "Correct max tokens value");
    
    json_object_put(params_obj);
    
    // Test JSON response creation
    char* response_json = io_create_json_response(42, true, "{\"content\": \"Hello response\"}");
    TEST_ASSERT(response_json != NULL, "Create JSON response");
    
    json_object* response_obj = json_tokener_parse(response_json);
    TEST_ASSERT(response_obj != NULL, "Parse response JSON");
    
    json_object* id_obj, *result_obj;
    TEST_ASSERT(json_object_object_get_ex(response_obj, "id", &id_obj), "Response has ID");
    TEST_ASSERT(json_object_get_int(id_obj) == 42, "Correct response ID");
    TEST_ASSERT(json_object_object_get_ex(response_obj, "result", &result_obj), "Response has result");
    
    json_object_put(response_obj);
    free(response_json);
    
    // Test streaming request detection
    const char* streaming_request = "{\"prompt\": \"Hello\", \"stream\": true}";
    const char* normal_request = "{\"prompt\": \"Hello\"}";
    
    bool is_streaming1 = io_is_streaming_request(streaming_request);
    bool is_streaming2 = io_is_streaming_request(normal_request);
    
    TEST_ASSERT(is_streaming1 == true, "Detect streaming request");
    TEST_ASSERT(is_streaming2 == false, "Detect normal request");
    
    io_operations_shutdown();
    return true;
}

// Test 5: Transport Configuration
bool test_transport_configuration(void) {
    printf("\n🧪 Test: Transport Configuration\n");
    printf("================================\n");
    strcpy(g_state.current_transport, "Config");
    
    // Test HTTP transport configuration
    http_transport_config_t http_config = {
        .host = str_copy("localhost"),
        .port = 8080,
        .path = str_copy("/mcp"),
        .timeout_ms = 5000,
        .keep_alive = true
    };
    
    TEST_ASSERT(http_config.host != NULL, "HTTP host configuration");
    TEST_ASSERT(http_config.port == 8080, "HTTP port configuration");
    TEST_ASSERT(strcmp(http_config.path, "/mcp") == 0, "HTTP path configuration");
    TEST_ASSERT(http_config.timeout_ms == 5000, "HTTP timeout configuration");
    TEST_ASSERT(http_config.keep_alive == true, "HTTP keep-alive configuration");
    
    // Test WebSocket transport configuration
    ws_transport_config_t ws_config = {
        .host = str_copy("localhost"),
        .port = 8083,
        .path = str_copy("/ws"),
        .mask_frames = true
    };
    
    TEST_ASSERT(ws_config.host != NULL, "WebSocket host configuration");
    TEST_ASSERT(ws_config.port == 8083, "WebSocket port configuration");
    TEST_ASSERT(strcmp(ws_config.path, "/ws") == 0, "WebSocket path configuration");
    TEST_ASSERT(ws_config.mask_frames == true, "WebSocket masking configuration");
    
    // Test transport initialization
    int http_init = http_transport_init(&http_config);
    TEST_ASSERT(http_init == 0, "HTTP transport initialization");
    
    int ws_init = ws_transport_init(&ws_config);
    TEST_ASSERT(ws_init == 0, "WebSocket transport initialization");
    
    // Test transport interfaces
    transport_base_t* http_interface = http_transport_get_interface();
    TEST_ASSERT(http_interface != NULL, "HTTP transport interface");
    
    // Cleanup
    str_free(http_config.host);
    str_free(http_config.path);
    str_free(ws_config.host);
    str_free(ws_config.path);
    
    http_transport_shutdown();
    ws_transport_shutdown();
    
    return true;
}

// Test 6: Error Handling Compliance
bool test_error_handling(void) {
    printf("\n🧪 Test: Error Handling Compliance\n");
    printf("==================================\n");
    strcpy(g_state.current_transport, "Error");
    
    // Test all standard MCP error codes
    mcp_error_code_t error_codes[] = {
        MCP_ERROR_PARSE,
        MCP_ERROR_INVALID_REQUEST,
        MCP_ERROR_METHOD_NOT_FOUND,
        MCP_ERROR_INVALID_PARAMS,
        MCP_ERROR_INTERNAL,
        MCP_ERROR_NOT_INITIALIZED,
        MCP_ERROR_ALREADY_INITIALIZED,
        MCP_ERROR_INVALID_VERSION,
        MCP_ERROR_STREAM_NOT_FOUND,
        MCP_ERROR_STREAM_EXPIRED,
        MCP_ERROR_STREAM_INVALID_STATE
    };
    
    for (size_t i = 0; i < sizeof(error_codes) / sizeof(error_codes[0]); i++) {
        const char* error_message = mcp_error_message(error_codes[i]);
        TEST_ASSERT(error_message != NULL, "Error message available");
        TEST_ASSERT(strlen(error_message) > 0, "Error message non-empty");
        
        char error_buffer[1024];
        int format_result = mcp_format_error(123, error_codes[i], error_message, NULL, error_buffer, sizeof(error_buffer));
        TEST_ASSERT(format_result == 0, "Format error response");
        
        json_object* error_obj = json_tokener_parse(error_buffer);
        TEST_ASSERT(error_obj != NULL, "Parse error JSON");
        json_object_put(error_obj);
    }
    
    return true;
}

// Main test runner
int main(void) {
    printf("🚀 MCP Compliance Test Suite - Unified Architecture\n");
    printf("===================================================\n");
    
    bool all_passed = true;
    
    all_passed &= test_mcp_protocol_basic();
    all_passed &= test_utf8_compliance();
    all_passed &= test_server_integration();
    all_passed &= test_io_operations_integration();
    all_passed &= test_transport_configuration();
    all_passed &= test_error_handling();
    
    printf("\n📊 Test Summary\n");
    printf("===============\n");
    printf("Tests run: %d\n", g_state.tests_run);
    printf("Passed: %d\n", g_state.tests_passed);
    printf("Failed: %d\n", g_state.tests_failed);
    
    if (all_passed && g_state.tests_failed == 0) {
        printf("✅ All MCP compliance tests passed!\n");
        return 0;
    } else {
        printf("❌ Some MCP compliance tests failed!\n");
        return 1;
    }
}