#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <json-c/json.h>
#include "../src/nano/core/nano/nano.h"
#include "../src/nano/transport/mcp_base/mcp_base.h"

/*
 * STREAMING CLIENT TEST
 * 
 * This test simulates a real client using the NANO system:
 * Client → NANO → IO → RKLLM → RKLLM Callback → IO Callback → NANO Callback → Client
 * 
 * Tests:
 * 1. Model initialization
 * 2. Synchronous inference 
 * 3. Streaming inference with real-time chunks
 * 4. Callback validation
 */

#define TEST_MODEL_PATH "models/qwen3/model.rkllm"

// Global test state
typedef struct {
    int responses_received;
    int chunks_received;
    bool streaming_complete;
    bool init_complete;
    bool sync_complete;
    char last_response[4096];
    char accumulated_text[8192];
    int handle_id;  // Store the handle_id from init
} streaming_test_state_t;

static streaming_test_state_t g_test_state = {0};

// Client callback to receive responses from NANO
void streaming_client_callback(const mcp_message_t* response, void* userdata) {
    streaming_test_state_t* state = (streaming_test_state_t*)userdata;
    
    printf("\n🔄 === CALLBACK RECEIVED ===\n");
    if (!response) {
        printf("❌ Response is NULL\n");
        return;
    }
    
    printf("📥 Response Details:\n");
    printf("   - Type: %d\n", response->type);
    printf("   - ID: %d\n", response->id);
    printf("   - Method: %s\n", response->method ? response->method : "NULL");
    printf("   - Params: %s\n", response->params ? response->params : "NULL");
    
    if (!response->params) {
        printf("❌ Response params are NULL\n");
        return;
    }
    
    state->responses_received++;
    strncpy(state->last_response, response->params, sizeof(state->last_response) - 1);
    
    // Parse response to check type
    json_object *response_obj = json_tokener_parse(response->params);
    if (response_obj) {
        // Check for errors first
        json_object *error_obj;
        if (json_object_object_get_ex(response_obj, "error", &error_obj)) {
            printf("❌ Error response: %s\n", json_object_to_json_string(error_obj));
            json_object_put(response_obj);
            return;
        }
        
        json_object *result_obj;
        if (json_object_object_get_ex(response_obj, "result", &result_obj)) {
            json_object *final_obj;
            if (json_object_object_get_ex(result_obj, "final", &final_obj)) {
                // This is a streaming chunk
                json_object *data_obj;
                if (json_object_object_get_ex(result_obj, "data", &data_obj)) {
                    const char* chunk_data = json_object_get_string(data_obj);
                    bool is_final = json_object_get_boolean(final_obj);
                    
                    state->chunks_received++;
                    
                    // Accumulate text
                    strncat(state->accumulated_text, chunk_data, 
                           sizeof(state->accumulated_text) - strlen(state->accumulated_text) - 1);
                    
                    printf("📦 Chunk %d: '%s' (final: %s)\n", 
                           state->chunks_received, chunk_data, is_final ? "true" : "false");
                    
                    if (is_final) {
                        state->streaming_complete = true;
                        printf("✅ Streaming complete! Total chunks: %d\n", state->chunks_received);
                        printf("📝 Full text: %s\n", state->accumulated_text);
                    }
                }
            } else {
                // Regular response - check if it's init or sync response
                const char* response_str = json_object_to_json_string(result_obj);
                if (strstr(response_str, "handle_id")) {
                    state->init_complete = true;
                    
                    // Extract handle_id for future use
                    json_object *data_obj;
                    if (json_object_object_get_ex(result_obj, "data", &data_obj)) {
                        json_object *handle_obj;
                        if (json_object_object_get_ex(data_obj, "handle_id", &handle_obj)) {
                            state->handle_id = json_object_get_int(handle_obj);
                            printf("✅ Model initialization complete (handle_id: %d)\n", state->handle_id);
                        }
                    }
                } else {
                    state->sync_complete = true;
                    printf("✅ Synchronous inference complete\n");
                }
                printf("📨 Response: %s\n", response_str);
            }
        }
        json_object_put(response_obj);
    }
}

// Test model initialization
int test_model_init() {
    printf("\n🔧 Testing Model Initialization...\n");
    
    json_object *params = json_object_new_object();
    json_object *model_path = json_object_new_string(TEST_MODEL_PATH);
    json_object_object_add(params, "model_path", model_path);
    
    const char *params_str = json_object_to_json_string(params);
    
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 1,
        .method = "init",
        .params = strdup(params_str)
    };
    
    printf("\n📤 === SENDING REQUEST ===\n");
    printf("📤 Request Details:\n");
    printf("   - Type: %d\n", request.type);
    printf("   - ID: %d\n", request.id);
    printf("   - Method: %s\n", request.method ? request.method : "NULL");
    printf("   - Params: %s\n", request.params ? request.params : "NULL");
    
    // Use async processing to test callbacks
    int result = nano_process_message_async(&request);
    printf("📤 Request submission result: %d\n", result);
    
    json_object_put(params);
    free(request.params);
    
    if (result == 0) {
        printf("✅ Model init request submitted\n");
        
        // Wait for initialization callback
        int timeout = 30;
        while (!g_test_state.init_complete && timeout-- > 0) {
            sleep(1);
            if (timeout % 5 == 0) {
                printf("⏳ Waiting for init... %d seconds remaining\n", timeout);
            }
        }
        
        if (g_test_state.init_complete) {
            printf("✅ Model initialization test passed\n");
            return 0;
        } else {
            printf("❌ Model initialization timeout\n");
            return -1;
        }
    } else {
        printf("❌ Model init request failed\n");
        return -1;
    }
}

// Test synchronous inference
int test_sync_inference() {
    printf("\n🔄 Testing Synchronous Inference...\n");
    
    json_object *params = json_object_new_object();
    json_object *prompt = json_object_new_string("Hello, how are you today?");
    json_object *handle_id = json_object_new_int(g_test_state.handle_id);
    json_object_object_add(params, "prompt", prompt);
    json_object_object_add(params, "handle_id", handle_id);
    
    const char *params_str = json_object_to_json_string(params);
    
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 2,
        .method = "run",
        .params = strdup(params_str)
    };
    
    printf("\n📤 === SENDING SYNC INFERENCE REQUEST ===\n");
    printf("📤 Request Details:\n");
    printf("   - Type: %d\n", request.type);
    printf("   - ID: %d\n", request.id);
    printf("   - Method: %s\n", request.method ? request.method : "NULL");
    printf("   - Params: %s\n", request.params ? request.params : "NULL");
    
    // Use async processing for callback testing
    int result = nano_process_message_async(&request);
    printf("📤 Request submission result: %d\n", result);
    
    json_object_put(params);
    free(request.params);
    
    if (result == 0) {
        printf("✅ Sync inference request submitted\n");
        
        // Wait for response callback
        int timeout = 20;
        while (!g_test_state.sync_complete && timeout-- > 0) {
            sleep(1);
            if (timeout % 5 == 0) {
                printf("⏳ Waiting for sync response... %d seconds remaining\n", timeout);
            }
        }
        
        if (g_test_state.sync_complete) {
            printf("✅ Synchronous inference test passed\n");
            return 0;
        } else {
            printf("❌ Synchronous inference timeout\n");
            return -1;
        }
    } else {
        printf("❌ Sync inference request failed\n");
        return -1;
    }
}

// Test streaming inference
int test_streaming_inference() {
    printf("\n🌊 Testing Streaming Inference...\n");
    
    // Reset streaming state
    g_test_state.chunks_received = 0;
    g_test_state.streaming_complete = false;
    memset(g_test_state.accumulated_text, 0, sizeof(g_test_state.accumulated_text));
    
    json_object *params = json_object_new_object();
    json_object *prompt = json_object_new_string("Tell me a short story about a robot learning to be human.");
    json_object *stream = json_object_new_boolean(true);
    json_object *handle_id = json_object_new_int(g_test_state.handle_id);
    json_object_object_add(params, "prompt", prompt);
    json_object_object_add(params, "stream", stream);
    json_object_object_add(params, "handle_id", handle_id);
    
    const char *params_str = json_object_to_json_string(params);
    
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 3,
        .method = "run",
        .params = strdup(params_str)
    };
    
    printf("\n📤 === SENDING STREAMING REQUEST ===\n");
    printf("📤 Request Details:\n");
    printf("   - Type: %d\n", request.type);
    printf("   - ID: %d\n", request.id);
    printf("   - Method: %s\n", request.method ? request.method : "NULL");
    printf("   - Params: %s\n", request.params ? request.params : "NULL");
    
    // Use async processing for streaming
    int result = nano_process_message_async(&request);
    printf("📤 Request submission result: %d\n", result);
    
    json_object_put(params);
    free(request.params);
    
    if (result == 0) {
        printf("✅ Streaming inference request submitted\n");
        
        // Wait for streaming to complete
        int timeout = 45; // Longer timeout for streaming
        while (!g_test_state.streaming_complete && timeout-- > 0) {
            sleep(1);
            if (timeout % 10 == 0) {
                printf("⏳ Streaming in progress... %d chunks received, %d seconds remaining\n", 
                       g_test_state.chunks_received, timeout);
            }
        }
        
        if (g_test_state.streaming_complete) {
            printf("✅ Streaming inference test passed!\n");
            printf("   Total chunks received: %d\n", g_test_state.chunks_received);
            printf("   Total text length: %zu characters\n", strlen(g_test_state.accumulated_text));
            return 0;
        } else {
            printf("⚠️  Streaming test timeout (received %d chunks)\n", g_test_state.chunks_received);
            return -1;
        }
    } else {
        printf("❌ Streaming inference request failed\n");
        return -1;
    }
}

// Main streaming client test
void test_streaming_client() {
    printf("🚀 Streaming Client Test - Full Workflow\n");
    printf("Architecture: Client → NANO → IO → RKLLM → Streaming Callbacks → Client\n");
    
    // Initialize NANO system
    printf("\n🔧 Initializing NANO system...\n");
    if (nano_init() != 0) {
        printf("❌ Failed to initialize NANO\n");
        return;
    }
    printf("✅ NANO system initialized\n");
    
    // Set up response callback
    nano_set_response_callback(streaming_client_callback, &g_test_state);
    printf("✅ Response callback registered\n");
    
    // Run test sequence
    int test_results = 0;
    
    // Test 1: Model initialization
    if (test_model_init() != 0) {
        test_results++;
    }
    
    // Test 2: Synchronous inference
    if (test_sync_inference() != 0) {
        test_results++;
    }
    
    // Test 3: Streaming inference
    if (test_streaming_inference() != 0) {
        test_results++;
    }
    
    // Cleanup
    printf("\n🔧 Shutting down...\n");
    nano_shutdown();
    printf("✅ Shutdown complete\n");
    
    // Summary
    printf("\n📊 Streaming Client Test Summary:\n");
    printf("   Total responses received: %d\n", g_test_state.responses_received);
    printf("   Streaming chunks received: %d\n", g_test_state.chunks_received);
    printf("   Failed tests: %d/3\n", test_results);
    
    if (test_results == 0) {
        printf("\n🎉 Streaming client test passed! Architecture is working correctly.\n");
    } else {
        printf("\n❌ %d test(s) failed. Check output above for details.\n", test_results);
    }
}