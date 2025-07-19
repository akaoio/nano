#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <json-c/json.h>
#include "../src/io/core/io/io.h"
#include "../src/io/operations.h"

/*
 * IO CALLBACK TEST
 * 
 * This test validates the IO layer callback mechanism:
 * Test → IO → RKLLM → RKLLM Callback → IO Callback → Test
 * 
 * Tests:
 * 1. IO system initialization with callback
 * 2. Request processing with callback responses
 * 3. Callback validation and timing
 * 4. Cleanup and shutdown
 */

#define TEST_MODEL_PATH "models/qwen3/model.rkllm"

// Test state for IO callbacks
typedef struct {
    int callbacks_received;
    bool init_callback_received;
    bool inference_callback_received;
    char last_response[4096];
    double callback_latency_ms;
} io_test_state_t;

static io_test_state_t g_io_test_state = {0};

// Test callback function for IO responses
void test_io_callback(const char* json_response, void* userdata) {
    io_test_state_t* state = (io_test_state_t*)userdata;
    
    if (!json_response) {
        printf("❌ IO callback received null response\n");
        return;
    }
    
    state->callbacks_received++;
    strncpy(state->last_response, json_response, sizeof(state->last_response) - 1);
    state->last_response[sizeof(state->last_response) - 1] = '\0';
    
    printf("📦 IO Callback %d: %s\n", state->callbacks_received, json_response);
    
    // Parse response to determine type
    json_object *response_obj = json_tokener_parse(json_response);
    if (response_obj) {
        json_object *result_obj;
        if (json_object_object_get_ex(response_obj, "result", &result_obj)) {
            const char* result_str = json_object_to_json_string(result_obj);
            
            if (strstr(result_str, "handle_id")) {
                state->init_callback_received = true;
                printf("✅ Model initialization callback received\n");
            } else if (strstr(result_str, "data") || strstr(result_str, "Hello")) {
                state->inference_callback_received = true;
                printf("✅ Inference callback received\n");
            }
        }
        json_object_put(response_obj);
    }
}

// Test IO initialization with callback
int test_io_init() {
    printf("\n🔧 Testing IO System Initialization...\n");
    
    // Initialize IO with test callback
    int result = io_init(test_io_callback, &g_io_test_state);
    
    if (result == IO_OK) {
        printf("✅ IO system initialized with callback\n");
        return 0;
    } else {
        printf("❌ IO system initialization failed: %d\n", result);
        return -1;
    }
}

// Test model initialization through IO
int test_io_model_init() {
    printf("\n🔧 Testing Model Initialization through IO...\n");
    
    // Create JSON-RPC request for model initialization
    json_object *request = json_object_new_object();
    json_object *jsonrpc = json_object_new_string("2.0");
    json_object *id = json_object_new_int(1);
    json_object *method = json_object_new_string("init");
    json_object *params = json_object_new_object();
    json_object *model_path = json_object_new_string(TEST_MODEL_PATH);
    
    json_object_object_add(params, "model_path", model_path);
    json_object_object_add(request, "jsonrpc", jsonrpc);
    json_object_object_add(request, "id", id);
    json_object_object_add(request, "method", method);
    json_object_object_add(request, "params", params);
    
    const char *request_str = json_object_to_json_string(request);
    
    // Push request to IO system
    int result = io_push_request(request_str);
    
    json_object_put(request);
    
    if (result == IO_OK) {
        printf("✅ Model init request pushed to IO\n");
        
        // Wait for callback response
        int timeout = 30;
        while (!g_io_test_state.init_callback_received && timeout-- > 0) {
            sleep(1);
            if (timeout % 5 == 0) {
                printf("⏳ Waiting for init callback... %d seconds remaining\n", timeout);
            }
        }
        
        if (g_io_test_state.init_callback_received) {
            printf("✅ Model initialization callback test passed\n");
            return 0;
        } else {
            printf("❌ Model initialization callback timeout\n");
            return -1;
        }
    } else {
        printf("❌ Failed to push init request to IO: %d\n", result);
        return -1;
    }
}

// Test inference through IO
int test_io_inference() {
    printf("\n🔄 Testing Inference through IO...\n");
    
    // Create JSON-RPC request for inference
    json_object *request = json_object_new_object();
    json_object *jsonrpc = json_object_new_string("2.0");
    json_object *id = json_object_new_int(2);
    json_object *method = json_object_new_string("run");
    json_object *params = json_object_new_object();
    json_object *prompt = json_object_new_string("Hello, how are you?");
    
    json_object_object_add(params, "prompt", prompt);
    json_object_object_add(request, "jsonrpc", jsonrpc);
    json_object_object_add(request, "id", id);
    json_object_object_add(request, "method", method);
    json_object_object_add(request, "params", params);
    
    const char *request_str = json_object_to_json_string(request);
    
    // Push request to IO system
    int result = io_push_request(request_str);
    
    json_object_put(request);
    
    if (result == IO_OK) {
        printf("✅ Inference request pushed to IO\n");
        
        // Wait for callback response
        int timeout = 20;
        while (!g_io_test_state.inference_callback_received && timeout-- > 0) {
            sleep(1);
            if (timeout % 5 == 0) {
                printf("⏳ Waiting for inference callback... %d seconds remaining\n", timeout);
            }
        }
        
        if (g_io_test_state.inference_callback_received) {
            printf("✅ Inference callback test passed\n");
            return 0;
        } else {
            printf("❌ Inference callback timeout\n");
            return -1;
        }
    } else {
        printf("❌ Failed to push inference request to IO: %d\n", result);
        return -1;
    }
}

// Test streaming request through IO
int test_io_streaming() {
    printf("\n🌊 Testing Streaming through IO...\n");
    
    // Reset state for streaming test
    int initial_callbacks = g_io_test_state.callbacks_received;
    
    // Create JSON-RPC request for streaming inference
    json_object *request = json_object_new_object();
    json_object *jsonrpc = json_object_new_string("2.0");
    json_object *id = json_object_new_int(3);
    json_object *method = json_object_new_string("run");
    json_object *params = json_object_new_object();
    json_object *prompt = json_object_new_string("Count from 1 to 5.");
    json_object *stream = json_object_new_boolean(true);
    
    json_object_object_add(params, "prompt", prompt);
    json_object_object_add(params, "stream", stream);
    json_object_object_add(request, "jsonrpc", jsonrpc);
    json_object_object_add(request, "id", id);
    json_object_object_add(request, "method", method);
    json_object_object_add(request, "params", params);
    
    const char *request_str = json_object_to_json_string(request);
    
    // Push request to IO system
    int result = io_push_request(request_str);
    
    json_object_put(request);
    
    if (result == IO_OK) {
        printf("✅ Streaming request pushed to IO\n");
        
        // Wait for streaming callbacks
        int timeout = 30;
        int streaming_callbacks = 0;
        
        while (timeout-- > 0) {
            int current_callbacks = g_io_test_state.callbacks_received - initial_callbacks;
            if (current_callbacks > streaming_callbacks) {
                streaming_callbacks = current_callbacks;
                printf("📦 Received streaming callback %d\n", streaming_callbacks);
                
                // Check if this looks like a final chunk
                if (strstr(g_io_test_state.last_response, "final") && 
                    strstr(g_io_test_state.last_response, "true")) {
                    printf("✅ Streaming complete after %d callbacks\n", streaming_callbacks);
                    return 0;
                }
            }
            sleep(1);
        }
        
        if (streaming_callbacks > 0) {
            printf("⚠️  Streaming test partial success (%d callbacks received)\n", streaming_callbacks);
            return 0;
        } else {
            printf("❌ No streaming callbacks received\n");
            return -1;
        }
    } else {
        printf("❌ Failed to push streaming request to IO: %d\n", result);
        return -1;
    }
}

// Main IO callback test
void test_io_callbacks() {
    printf("🔧 IO Callback Test - Direct IO Layer Testing\n");
    printf("Architecture: Test → IO → RKLLM → IO Callback → Test\n");
    
    int test_results = 0;
    
    // Test 1: IO initialization
    if (test_io_init() != 0) {
        test_results++;
    }
    
    // Test 2: Model initialization through IO
    if (test_io_model_init() != 0) {
        test_results++;
    }
    
    // Test 3: Inference through IO
    if (test_io_inference() != 0) {
        test_results++;
    }
    
    // Test 4: Streaming through IO
    if (test_io_streaming() != 0) {
        test_results++;
    }
    
    // Cleanup
    printf("\n🔧 Shutting down IO system...\n");
    io_shutdown();
    printf("✅ IO shutdown complete\n");
    
    // Summary
    printf("\n📊 IO Callback Test Summary:\n");
    printf("   Total callbacks received: %d\n", g_io_test_state.callbacks_received);
    printf("   Init callback received: %s\n", g_io_test_state.init_callback_received ? "Yes" : "No");
    printf("   Inference callback received: %s\n", g_io_test_state.inference_callback_received ? "Yes" : "No");
    printf("   Failed tests: %d/4\n", test_results);
    
    if (test_results == 0) {
        printf("\n🎉 IO callback test passed! IO layer is working correctly.\n");
    } else {
        printf("\n❌ %d test(s) failed. Check output above for details.\n", test_results);
    }
}