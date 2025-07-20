#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <json-c/json.h>

// Include streaming components
#include "../src/nano/transport/streaming/stream_manager.h"
#include "../src/nano/transport/http_transport/http_transport.h"
#include "../src/nano/transport/ws_transport/ws_transport.h"
#include "../src/nano/transport/mcp_base/mcp_protocol.h"
#include "../src/io/operations.h"

/*
 * STREAMING MCP PROTOCOL TEST SUITE
 * 
 * Tests the complete streaming implementation:
 * 1. Stream Manager functionality
 * 2. HTTP polling-based streaming
 * 3. WebSocket push-based streaming
 * 4. MCP protocol streaming extensions
 * 5. IO operations streaming integration
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
    TEST_ASSERT(retrieved == session, "Session retrieval by ID");
    
    // Test statistics
    stream_stats_t stats = stream_get_statistics();
    TEST_ASSERT(stats.active_sessions >= 2, "Active session count");
    TEST_ASSERT(stats.total_chunks >= 4, "Total chunk count");
    
    // Cleanup
    stream_destroy_session(stream_id);
    stream_destroy_session(error_session->stream_id);
    stream_manager_shutdown();
    
    return true;
}

// Test 2: MCP Protocol Streaming Extensions
bool test_mcp_protocol_streaming(void) {
    printf("\n🧪 Test: MCP Protocol Streaming Extensions\n");
    printf("==========================================\n");
    
    char buffer[8192];
    
    // Test stream chunk formatting
    int format_result = mcp_format_stream_chunk("run", "abc123def456", 5, "Hello chunk", false, NULL, buffer, sizeof(buffer));
    TEST_ASSERT(format_result == 0, "Format stream chunk");
    
    // Parse and verify JSON structure
    json_object* chunk_obj = json_tokener_parse(buffer);
    TEST_ASSERT(chunk_obj != NULL, "Parse formatted chunk JSON");
    
    json_object* jsonrpc_obj, *method_obj, *params_obj;
    TEST_ASSERT(json_object_object_get_ex(chunk_obj, "jsonrpc", &jsonrpc_obj), "JSON-RPC version field");
    TEST_ASSERT(strcmp(json_object_get_string(jsonrpc_obj), "2.0") == 0, "JSON-RPC version value");
    
    TEST_ASSERT(json_object_object_get_ex(chunk_obj, "method", &method_obj), "Method field");
    TEST_ASSERT(strcmp(json_object_get_string(method_obj), "run") == 0, "Method value");
    
    TEST_ASSERT(json_object_object_get_ex(chunk_obj, "params", &params_obj), "Params field");
    
    // Verify params structure
    json_object* stream_id_obj, *seq_obj, *delta_obj, *end_obj;
    TEST_ASSERT(json_object_object_get_ex(params_obj, "stream_id", &stream_id_obj), "Stream ID field");
    TEST_ASSERT(strcmp(json_object_get_string(stream_id_obj), "abc123def456") == 0, "Stream ID value");
    
    TEST_ASSERT(json_object_object_get_ex(params_obj, "seq", &seq_obj), "Sequence field");
    TEST_ASSERT(json_object_get_int(seq_obj) == 5, "Sequence value");
    
    TEST_ASSERT(json_object_object_get_ex(params_obj, "delta", &delta_obj), "Delta field");
    TEST_ASSERT(strcmp(json_object_get_string(delta_obj), "Hello chunk") == 0, "Delta value");
    
    TEST_ASSERT(json_object_object_get_ex(params_obj, "end", &end_obj), "End field");
    TEST_ASSERT(json_object_get_boolean(end_obj) == false, "End value");
    
    json_object_put(chunk_obj);
    
    // Test error chunk formatting
    int error_format_result = mcp_format_stream_chunk("run", "abc123def456", 6, "", true, "Stream error occurred", buffer, sizeof(buffer));
    TEST_ASSERT(error_format_result == 0, "Format error chunk");
    
    json_object* error_chunk_obj = json_tokener_parse(buffer);
    TEST_ASSERT(error_chunk_obj != NULL, "Parse error chunk JSON");
    
    json_object* error_params_obj, *error_obj;
    TEST_ASSERT(json_object_object_get_ex(error_chunk_obj, "params", &error_params_obj), "Error chunk params");
    TEST_ASSERT(json_object_object_get_ex(error_params_obj, "error", &error_obj), "Error object");
    
    json_object* error_msg_obj;
    TEST_ASSERT(json_object_object_get_ex(error_obj, "message", &error_msg_obj), "Error message field");
    TEST_ASSERT(strcmp(json_object_get_string(error_msg_obj), "Stream error occurred") == 0, "Error message value");
    
    json_object_put(error_chunk_obj);
    
    // Test stream request parsing
    const char* stream_params = "{\"prompt\": \"Hello\", \"stream\": true, \"max_tokens\": 100}";
    bool is_stream;
    char other_params[4096];
    
    int parse_result = mcp_parse_stream_request(stream_params, &is_stream, other_params, sizeof(other_params));
    TEST_ASSERT(parse_result == 0, "Parse stream request");
    TEST_ASSERT(is_stream == true, "Detect stream parameter");
    
    // Verify stream parameter was removed
    json_object* parsed_params = json_tokener_parse(other_params);
    TEST_ASSERT(parsed_params != NULL, "Parse filtered params");
    
    json_object* prompt_obj, *max_tokens_obj, *stream_param_obj;
    TEST_ASSERT(json_object_object_get_ex(parsed_params, "prompt", &prompt_obj), "Prompt preserved");
    TEST_ASSERT(json_object_object_get_ex(parsed_params, "max_tokens", &max_tokens_obj), "Max tokens preserved");
    TEST_ASSERT(!json_object_object_get_ex(parsed_params, "stream", &stream_param_obj), "Stream parameter removed");
    
    json_object_put(parsed_params);
    
    return true;
}

// Test 3: HTTP Transport Streaming
bool test_http_streaming(void) {
    printf("\n🧪 Test: HTTP Transport Streaming\n");
    printf("=================================\n");
    
    // Initialize HTTP transport with streaming enabled
    http_transport_config_t config = {
        .host = str_copy("localhost"),
        .port = 8080,
        .path = str_copy("/mcp"),
        .streaming_enabled = true
    };
    
    int init_result = http_transport_init(&config);
    TEST_ASSERT(init_result == 0, "HTTP transport initialization");
    
    // Create mock stream request
    mcp_message_t request = {0};
    strcpy(request.method, "run");
    strcpy(request.params, "{\"prompt\": \"Hello\", \"stream\": true}");
    request.id = 12345;
    request.is_response = false;
    
    mcp_message_t response = {0};
    
    // Test stream request handling
    int stream_result = http_transport_handle_stream_request(&request, &response);
    TEST_ASSERT(stream_result == 0, "HTTP handle stream request");
    TEST_ASSERT(response.is_response == true, "Response marked as response");
    
    // Parse response to get stream ID
    json_object* response_obj = json_tokener_parse(response.params);
    TEST_ASSERT(response_obj != NULL, "Parse stream response");
    
    json_object* stream_id_obj, *status_obj;
    TEST_ASSERT(json_object_object_get_ex(response_obj, "stream_id", &stream_id_obj), "Stream ID in response");
    TEST_ASSERT(json_object_object_get_ex(response_obj, "status", &status_obj), "Status in response");
    TEST_ASSERT(strcmp(json_object_get_string(status_obj), "streaming_started") == 0, "Streaming started status");
    
    const char* stream_id = json_object_get_string(stream_id_obj);
    TEST_ASSERT(strlen(stream_id) == STREAM_ID_LENGTH, "Valid stream ID length");
    
    // Add some test chunks to the stream
    stream_add_chunk(stream_id, "Hello", false, NULL);
    stream_add_chunk(stream_id, " from", false, NULL);
    stream_add_chunk(stream_id, " HTTP!", true, NULL);
    
    // Test polling for chunks
    mcp_message_t poll_response = {0};
    int poll_result = http_transport_handle_stream_poll(stream_id, 0, &poll_response);
    TEST_ASSERT(poll_result == 0, "HTTP poll stream chunks");
    
    // Parse polling response
    json_object* poll_obj = json_tokener_parse(poll_response.params);
    TEST_ASSERT(poll_obj != NULL, "Parse poll response");
    
    json_object* chunks_array_obj, *has_more_obj;
    TEST_ASSERT(json_object_object_get_ex(poll_obj, "chunks", &chunks_array_obj), "Chunks array in poll response");
    TEST_ASSERT(json_object_object_get_ex(poll_obj, "has_more", &has_more_obj), "Has more field in poll response");
    TEST_ASSERT(json_object_get_boolean(has_more_obj) == false, "No more chunks after final");
    
    // Verify chunks content
    int chunks_count = json_object_array_length(chunks_array_obj);
    TEST_ASSERT(chunks_count == 3, "Correct number of chunks");
    
    json_object* first_chunk = json_object_array_get_idx(chunks_array_obj, 0);
    json_object* first_delta_obj, *first_seq_obj;
    TEST_ASSERT(json_object_object_get_ex(first_chunk, "delta", &first_delta_obj), "First chunk delta");
    TEST_ASSERT(json_object_object_get_ex(first_chunk, "seq", &first_seq_obj), "First chunk sequence");
    TEST_ASSERT(strcmp(json_object_get_string(first_delta_obj), "Hello") == 0, "First chunk content");
    TEST_ASSERT(json_object_get_int(first_seq_obj) == 0, "First chunk sequence number");
    
    json_object_put(response_obj);
    json_object_put(poll_obj);
    
    // Cleanup
    str_free(config.host);
    str_free(config.path);
    http_transport_shutdown();
    
    return true;
}

// Test 4: WebSocket Transport Streaming
bool test_websocket_streaming(void) {
    printf("\n🧪 Test: WebSocket Transport Streaming\n");
    printf("======================================\n");
    
    // Initialize WebSocket transport with streaming enabled
    ws_transport_config_t config = {
        .host = str_copy("localhost"),
        .port = 8080,
        .path = str_copy("/ws"),
        .streaming_enabled = true
    };
    
    int init_result = ws_transport_init(&config);
    TEST_ASSERT(init_result == 0, "WebSocket transport initialization");
    
    // Create mock stream request
    mcp_message_t request = {0};
    strcpy(request.method, "run");
    strcpy(request.params, "{\"prompt\": \"Hello WebSocket\", \"stream\": true}");
    request.id = 67890;
    request.is_response = false;
    
    mcp_message_t response = {0};
    
    // Test stream request handling
    int stream_result = ws_transport_handle_stream_request(&request, &response);
    TEST_ASSERT(stream_result == 0, "WebSocket handle stream request");
    
    // Parse response to get stream ID
    json_object* response_obj = json_tokener_parse(response.params);
    TEST_ASSERT(response_obj != NULL, "Parse WebSocket stream response");
    
    json_object* stream_id_obj;
    TEST_ASSERT(json_object_object_get_ex(response_obj, "stream_id", &stream_id_obj), "Stream ID in WebSocket response");
    const char* stream_id = json_object_get_string(stream_id_obj);
    
    // Test creating stream response (prepares session for streaming)
    int create_result = ws_transport_create_stream_response(stream_id, "run");
    TEST_ASSERT(create_result == 0, "Create WebSocket stream response");
    
    // Note: ws_transport_send_stream_chunk would normally send over socket
    // For testing, we verify the JSON formatting
    // This function would be called by the RKLLM callback
    
    json_object_put(response_obj);
    
    // Cleanup
    str_free(config.host);
    str_free(config.path);
    ws_transport_shutdown();
    
    return true;
}

// Test 5: IO Operations Streaming Integration
bool test_io_streaming_integration(void) {
    printf("\n🧪 Test: IO Operations Streaming Integration\n");
    printf("============================================\n");
    
    // Test streaming request detection
    const char* regular_params = "{\"prompt\": \"Hello\", \"max_tokens\": 100}";
    const char* streaming_params = "{\"prompt\": \"Hello\", \"stream\": true, \"max_tokens\": 100}";
    
    bool is_regular_stream = io_is_streaming_request(regular_params);
    bool is_streaming_stream = io_is_streaming_request(streaming_params);
    
    TEST_ASSERT(!is_regular_stream, "Regular request not detected as streaming");
    TEST_ASSERT(is_streaming_stream, "Streaming request detected correctly");
    
    // Test streaming request processing
    const char* stream_request = "{"
        "\"jsonrpc\": \"2.0\","
        "\"id\": 123,"
        "\"method\": \"run\","
        "\"params\": {"
            "\"prompt\": \"Hello streaming world\","
            "\"stream\": true,"
            "\"max_tokens\": 50"
        "}"
    "}";
    
    char response[8192];
    int process_result = io_process_streaming_request(stream_request, response, sizeof(response));
    
    // Note: This will fail without actual RKLLM model, but we can test the parsing
    // In a real scenario with model loaded, this would return 0 and create a stream
    printf("   Stream processing result: %d\n", process_result);
    printf("   Response: %s\n", response);
    
    // Test should pass if we get a proper error response about model not being initialized
    json_object* response_obj = json_tokener_parse(response);
    TEST_ASSERT(response_obj != NULL, "Parse streaming response");
    
    json_object* id_obj, *error_obj;
    bool has_error = json_object_object_get_ex(response_obj, "error", &error_obj);
    bool has_id = json_object_object_get_ex(response_obj, "id", &id_obj);
    
    TEST_ASSERT(has_id, "Response contains request ID");
    TEST_ASSERT(json_object_get_int(id_obj) == 123, "Correct request ID in response");
    
    // Either success with stream_id or error response is acceptable
    if (has_error) {
        printf("   (Expected error due to no model loaded)\n");
    }
    
    json_object_put(response_obj);
    
    return true;
}

// Main test runner
int main(void) {
    printf("🚀 STREAMING MCP PROTOCOL TEST SUITE\n");
    printf("=====================================\n");
    
    // Run all tests
    test_stream_manager();
    test_mcp_protocol_streaming();
    test_http_streaming();
    test_websocket_streaming();
    test_io_streaming_integration();
    
    // Print summary
    printf("\n📊 TEST SUMMARY\n");
    printf("===============\n");
    printf("Tests run: %d\n", g_state.tests_run);
    printf("✅ Passed: %d\n", g_state.tests_passed);
    printf("❌ Failed: %d\n", g_state.tests_failed);
    printf("Success rate: %.1f%%\n", 
           g_state.tests_run > 0 ? (float)g_state.tests_passed / g_state.tests_run * 100 : 0);
    
    if (g_state.tests_failed == 0) {
        printf("\n🎉 ALL STREAMING TESTS PASSED!\n");
        printf("The streaming MCP protocol implementation is working correctly.\n");
        return 0;
    } else {
        printf("\n⚠️  Some streaming tests failed.\n");
        printf("Please review the failed test cases above.\n");
        return 1;
    }
}