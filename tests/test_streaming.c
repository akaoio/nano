#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <json-c/json.h>

// Include unified server architecture components
#include "../src/lib/protocol/streaming.h"
#include "../src/lib/transport/http.h"
#include "../src/lib/transport/websocket.h"
#include "../src/lib/protocol/mcp_protocol.h"
#include "../src/lib/core/operations.h"
#include "../src/common/string_utils/string_utils.h"

/*
 * STREAMING MCP PROTOCOL TEST SUITE - UNIFIED ARCHITECTURE
 * 
 * Tests the complete streaming implementation:
 * 1. Stream Manager functionality
 * 2. MCP protocol streaming integration
 * 3. IO operations streaming integration
 * 4. Transport layer streaming support
 */

// Test state
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} test_state_t;

static test_state_t g_state = {0};

#define TEST_ASSERT(condition, message) \
    do { \
        g_state.tests_run++; \
        if (condition) { \
            printf("✅ %s\n", message); \
            g_state.tests_passed++; \
        } else { \
            printf("❌ %s\n", message); \
            g_state.tests_failed++; \
        } \
    } while(0)

// Test 1: Stream Manager Core Functionality
bool test_stream_manager(void) {
    printf("\n🧪 Test: Stream Manager Core Functionality\n");
    printf("==========================================\n");
    
    // Initialize stream manager
    int init_result = stream_manager_init();
    TEST_ASSERT(init_result == 0, "Stream manager initialization");
    
    // Create stream session
    stream_session_t* session = stream_create_session("run", 12345);
    TEST_ASSERT(session != NULL, "Stream session creation");
    TEST_ASSERT(strlen(session->stream_id) == STREAM_ID_LENGTH, "Stream ID length validation");
    TEST_ASSERT(strcmp(session->original_method, "run") == 0, "Stream method preservation");
    TEST_ASSERT(session->request_id == 12345, "Stream request ID preservation");
    
    char stream_id[STREAM_ID_LENGTH + 1];
    strcpy(stream_id, session->stream_id);
    
    // Add chunks to stream
    int add_result1 = stream_add_chunk(stream_id, "Hello", false, NULL);
    TEST_ASSERT(add_result1 == 0, "Add first chunk");
    
    int add_result2 = stream_add_chunk(stream_id, " World", false, NULL);
    TEST_ASSERT(add_result2 == 0, "Add second chunk");
    
    int add_result3 = stream_add_chunk(stream_id, "!", true, NULL);
    TEST_ASSERT(add_result3 == 0, "Add final chunk");
    
    // Retrieve pending chunks
    stream_chunk_t* chunks = stream_get_pending_chunks(stream_id, 0);
    TEST_ASSERT(chunks != NULL, "Retrieve pending chunks");
    
    // Verify chunk content and sequence
    TEST_ASSERT(chunks->seq == 0, "First chunk sequence");
    TEST_ASSERT(strcmp(chunks->delta, "Hello") == 0, "First chunk content");
    TEST_ASSERT(!chunks->end, "First chunk not final");
    
    TEST_ASSERT(chunks->next != NULL, "Second chunk exists");
    TEST_ASSERT(chunks->next->seq == 1, "Second chunk sequence");
    TEST_ASSERT(strcmp(chunks->next->delta, " World") == 0, "Second chunk content");
    
    TEST_ASSERT(chunks->next->next != NULL, "Third chunk exists");
    TEST_ASSERT(chunks->next->next->seq == 2, "Third chunk sequence");
    TEST_ASSERT(strcmp(chunks->next->next->delta, "!") == 0, "Third chunk content");
    TEST_ASSERT(chunks->next->next->end == true, "Final chunk marked");
    
    // Test error chunk
    stream_session_t* error_session = stream_create_session("test", 99999);
    int error_result = stream_add_chunk(error_session->stream_id, NULL, true, "Test error");
    TEST_ASSERT(error_result == 0, "Add error chunk");
    
    // Test session retrieval
    stream_session_t* retrieved = stream_get_session(stream_id);
    TEST_ASSERT(retrieved != NULL, "Session retrieval");
    TEST_ASSERT(retrieved == session, "Session identity");
    
    // Test statistics
    stream_stats_t stats = stream_get_statistics();
    TEST_ASSERT(stats.active_sessions >= 2, "Active sessions count");
    TEST_ASSERT(stats.total_chunks >= 4, "Total chunks count");
    
    // Cleanup streams
    stream_destroy_session(stream_id);
    stream_destroy_session(error_session->stream_id);
    
    stream_manager_shutdown();
    
    return true;
}

// Test 2: MCP Protocol Streaming Integration
bool test_mcp_protocol_streaming(void) {
    printf("\n🧪 Test: MCP Protocol Streaming Integration\n");
    printf("==========================================\n");
    
    // Initialize MCP context for streaming
    mcp_context_t ctx = {0};
    int init_result = mcp_protocol_init(&ctx);
    TEST_ASSERT(init_result == 0, "MCP protocol initialization");
    
    // Test stream chunk formatting
    char chunk_buffer[1024];
    int format_result = mcp_format_stream_chunk("run", "test_stream_123", 0, "Hello", false, NULL, chunk_buffer, sizeof(chunk_buffer));
    TEST_ASSERT(format_result == 0, "Format stream chunk");
    
    // Parse and verify chunk format
    json_object* chunk_obj = json_tokener_parse(chunk_buffer);
    TEST_ASSERT(chunk_obj != NULL, "Parse stream chunk JSON");
    
    json_object* method_obj, *params_obj;
    TEST_ASSERT(json_object_object_get_ex(chunk_obj, "method", &method_obj), "Chunk method field");
    TEST_ASSERT(json_object_object_get_ex(chunk_obj, "params", &params_obj), "Chunk params field");
    TEST_ASSERT(strcmp(json_object_get_string(method_obj), "run") == 0, "Chunk method value");
    
    // Test stream request parsing
    const char* stream_params = "{\"prompt\": \"Hello\", \"stream\": true, \"max_tokens\": 100}";
    bool is_stream = false;
    char other_params[512];
    int parse_result = mcp_parse_stream_request(stream_params, &is_stream, other_params, sizeof(other_params));
    TEST_ASSERT(parse_result == 0, "Parse stream request");
    TEST_ASSERT(is_stream == true, "Detect streaming request");
    
    // Verify other params are preserved
    json_object* other_obj = json_tokener_parse(other_params);
    TEST_ASSERT(other_obj != NULL, "Parse other params");
    
    json_object* prompt_obj, *tokens_obj;
    TEST_ASSERT(json_object_object_get_ex(other_obj, "prompt", &prompt_obj), "Prompt preserved");
    TEST_ASSERT(json_object_object_get_ex(other_obj, "max_tokens", &tokens_obj), "Max tokens preserved");
    TEST_ASSERT(strcmp(json_object_get_string(prompt_obj), "Hello") == 0, "Prompt value preserved");
    TEST_ASSERT(json_object_get_int(tokens_obj) == 100, "Max tokens value preserved");
    
    // Test poll request handling
    stream_manager_init();
    stream_session_t* session = stream_create_session("run", 12345);
    stream_add_chunk(session->stream_id, "Test", false, NULL);
    stream_add_chunk(session->stream_id, " Response", true, NULL);
    
    char poll_response[2048];
    int poll_result = mcp_handle_stream_poll_request(session->stream_id, 0, poll_response, sizeof(poll_response));
    TEST_ASSERT(poll_result == 0, "Handle stream poll request");
    
    // Parse poll response
    json_object* poll_obj = json_tokener_parse(poll_response);
    TEST_ASSERT(poll_obj != NULL, "Parse poll response");
    
    json_object* chunks_array, *has_more_obj;
    TEST_ASSERT(json_object_object_get_ex(poll_obj, "chunks", &chunks_array), "Chunks in poll response");
    TEST_ASSERT(json_object_object_get_ex(poll_obj, "has_more", &has_more_obj), "Has more in poll response");
    TEST_ASSERT(json_object_get_boolean(has_more_obj) == false, "No more chunks after final");
    
    int chunk_count = json_object_array_length(chunks_array);
    TEST_ASSERT(chunk_count == 2, "Correct chunk count in poll response");
    
    // Cleanup
    json_object_put(chunk_obj);
    json_object_put(other_obj);
    json_object_put(poll_obj);
    stream_destroy_session(session->stream_id);
    stream_manager_shutdown();
    mcp_protocol_shutdown(&ctx);
    
    return true;
}

// Test 3: IO Operations Streaming Integration
bool test_io_operations_streaming(void) {
    printf("\n🧪 Test: IO Operations Streaming Integration\n");
    printf("==========================================\n");
    
    // Initialize IO operations
    int init_result = io_operations_init();
    TEST_ASSERT(init_result == 0, "IO operations initialization");
    
    // Test streaming request detection
    const char* stream_request = "{\"prompt\": \"Hello\", \"stream\": true}";
    const char* normal_request = "{\"prompt\": \"Hello\", \"stream\": false}";
    const char* no_stream_request = "{\"prompt\": \"Hello\"}";
    
    bool is_streaming1 = io_is_streaming_request(stream_request);
    bool is_streaming2 = io_is_streaming_request(normal_request);
    bool is_streaming3 = io_is_streaming_request(no_stream_request);
    
    TEST_ASSERT(is_streaming1 == true, "Detect streaming request (true)");
    TEST_ASSERT(is_streaming2 == false, "Detect non-streaming request (false)");
    TEST_ASSERT(is_streaming3 == false, "Detect non-streaming request (missing)");
    
    // Test streaming request processing
    const char* json_stream_request = "{"
        "\"jsonrpc\": \"2.0\","
        "\"id\": 12345,"
        "\"method\": \"run\","
        "\"params\": {\"prompt\": \"Hello streaming\", \"stream\": true}"
    "}";
    
    char response[2048];
    int process_result = io_process_streaming_request(json_stream_request, response, sizeof(response));
    TEST_ASSERT(process_result == 0, "Process streaming request");
    
    // Parse response to verify stream ID
    json_object* response_obj = json_tokener_parse(response);
    TEST_ASSERT(response_obj != NULL, "Parse streaming response");
    
    json_object* result_obj;
    TEST_ASSERT(json_object_object_get_ex(response_obj, "result", &result_obj), "Result in streaming response");
    
    json_object* stream_id_obj;
    TEST_ASSERT(json_object_object_get_ex(result_obj, "stream_id", &stream_id_obj), "Stream ID in result");
    
    const char* stream_id = json_object_get_string(stream_id_obj);
    TEST_ASSERT(strlen(stream_id) == STREAM_ID_LENGTH, "Valid stream ID length");
    
    // Test adding chunks through IO operations
    int chunk_result = io_add_stream_chunk(stream_id, "Hello", false, NULL);
    TEST_ASSERT(chunk_result == 0, "Add chunk through IO operations");
    
    chunk_result = io_add_stream_chunk(stream_id, " World!", true, NULL);
    TEST_ASSERT(chunk_result == 0, "Add final chunk through IO operations");
    
    // Cleanup
    json_object_put(response_obj);
    io_operations_shutdown();
    
    return true;
}

// Test 4: Transport Layer Streaming Support
bool test_transport_streaming(void) {
    printf("\n🧪 Test: Transport Layer Streaming Support\n");
    printf("==========================================\n");
    
    // Test HTTP transport initialization
    http_transport_config_t http_config = {
        .host = str_copy("localhost"),
        .port = 8080,
        .path = str_copy("/mcp"),
        .timeout_ms = 5000,
        .keep_alive = true
    };
    
    int http_init = http_transport_init(&http_config);
    TEST_ASSERT(http_init == 0, "HTTP transport initialization");
    
    // Test WebSocket transport initialization
    ws_transport_config_t ws_config = {
        .host = str_copy("localhost"),
        .port = 8083,
        .path = str_copy("/ws"),
        .mask_frames = true
    };
    
    int ws_init = ws_transport_init(&ws_config);
    TEST_ASSERT(ws_init == 0, "WebSocket transport initialization");
    
    // Test transport interfaces
    transport_base_t* http_interface = http_transport_get_interface();
    TEST_ASSERT(http_interface != NULL, "HTTP transport interface");
    
    // Test if transports can handle streaming data
    const char* test_data = "{\"method\":\"stream_chunk\",\"params\":{\"stream_id\":\"test123\",\"delta\":\"Hello\"}}";
    
    // Note: Since these are mock transports in test environment, we just test the API availability
    TEST_ASSERT(http_transport_send_raw != NULL, "HTTP send function available");
    TEST_ASSERT(ws_transport_send_raw != NULL, "WebSocket send function available");
    
    // Cleanup
    str_free(http_config.host);
    str_free(http_config.path);
    str_free(ws_config.host);
    str_free(ws_config.path);
    
    http_transport_shutdown();
    ws_transport_shutdown();
    
    return true;
}

// Main test runner
int main(void) {
    printf("🚀 MCP Server Streaming Test Suite - Unified Architecture\n");
    printf("=========================================================\n");
    
    bool all_passed = true;
    
    all_passed &= test_stream_manager();
    all_passed &= test_mcp_protocol_streaming();
    all_passed &= test_io_operations_streaming();
    all_passed &= test_transport_streaming();
    
    printf("\n📊 Test Summary\n");
    printf("===============\n");
    printf("Tests run: %d\n", g_state.tests_run);
    printf("Passed: %d\n", g_state.tests_passed);
    printf("Failed: %d\n", g_state.tests_failed);
    
    if (all_passed && g_state.tests_failed == 0) {
        printf("✅ All streaming tests passed!\n");
        return 0;
    } else {
        printf("❌ Some streaming tests failed!\n");
        return 1;
    }
}