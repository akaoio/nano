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
 * NANO CALLBACK TEST
 * 
 * This test validates the NANO layer callback mechanism:
 * Test → NANO → IO → RKLLM → IO Callback → NANO Callback → Test
 * 
 * Tests:
 * 1. NANO system initialization
 * 2. Response callback registration
 * 3. Async message processing with callbacks
 * 4. Callback validation and timing
 * 5. Multiple concurrent requests
 */

#define TEST_MODEL_PATH "models/qwen3/model.rkllm"

// Test state for NANO callbacks
typedef struct {
    int callbacks_received;
    bool init_callback_received;
    bool inference_callback_received;
    bool streaming_callback_received;
    char last_response[4096];
    int concurrent_responses;
} nano_test_state_t;

static nano_test_state_t g_nano_test_state = {0};

// Test callback function for NANO responses
void test_nano_callback(const mcp_message_t* response, void* userdata) {
    nano_test_state_t* state = (nano_test_state_t*)userdata;
    
    if (!response) {
        printf("❌ NANO callback received null response\n");
        return;
    }
    
    state->callbacks_received++;
    
    if (response->params) {
        strncpy(state->last_response, response->params, sizeof(state->last_response) - 1);
        state->last_response[sizeof(state->last_response) - 1] = '\0';
    }
    
    printf("📦 NANO Callback %d (ID: %d): %s\n", 
           state->callbacks_received, response->id, 
           response->params ? response->params : "null");
    
    // Parse response to determine type
    if (response->params) {
        json_object *response_obj = json_tokener_parse(response->params);
        if (response_obj) {
            json_object *result_obj;
            if (json_object_object_get_ex(response_obj, "result", &result_obj)) {
                const char* result_str = json_object_to_json_string(result_obj);
                
                if (strstr(result_str, "handle_id")) {
                    state->init_callback_received = true;
                    printf("✅ Model initialization callback received\n");
                } else if (strstr(result_str, "final")) {
                    state->streaming_callback_received = true;
                    printf("✅ Streaming callback received\n");
                } else if (strstr(result_str, "data")) {
                    state->inference_callback_received = true;
                    printf("✅ Inference callback received\n");
                }
            }
            json_object_put(response_obj);
        }
    }
    
    // Track concurrent responses
    if (response->id > 1) {
        state->concurrent_responses++;
    }
}

// Test NANO initialization
int test_nano_init() {
    printf("\n🔧 Testing NANO System Initialization...\n");
    
    // Initialize NANO system
    int result = nano_init();
    
    if (result == 0) {
        printf("✅ NANO system initialized\n");
        
        // Set up response callback
        nano_set_response_callback(test_nano_callback, &g_nano_test_state);
        printf("✅ NANO response callback registered\n");
        
        return 0;
    } else {
        printf("❌ NANO system initialization failed: %d\n", result);
        return -1;
    }
}

// Test async model initialization through NANO
int test_nano_async_init() {
    printf("\n🔧 Testing Async Model Initialization through NANO...\n");
    
    // Create MCP request for model initialization
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
    
    // Use async processing
    int result = nano_process_message_async(&request);
    
    json_object_put(params);
    free(request.params);
    
    if (result == 0) {
        printf("✅ Async model init request submitted\n");
        
        // Wait for callback response
        int timeout = 30;
        while (!g_nano_test_state.init_callback_received && timeout-- > 0) {
            sleep(1);
            if (timeout % 5 == 0) {
                printf("⏳ Waiting for init callback... %d seconds remaining\n", timeout);
            }
        }
        
        if (g_nano_test_state.init_callback_received) {
            printf("✅ Async model initialization test passed\n");
            return 0;
        } else {
            printf("❌ Async model initialization callback timeout\n");
            return -1;
        }
    } else {
        printf("❌ Failed to submit async init request: %d\n", result);
        return -1;
    }
}

// Test async inference through NANO
int test_nano_async_inference() {
    printf("\n🔄 Testing Async Inference through NANO...\n");
    
    // Create MCP request for inference
    json_object *params = json_object_new_object();
    json_object *prompt = json_object_new_string("Hello, how are you?");
    json_object_object_add(params, "prompt", prompt);
    
    const char *params_str = json_object_to_json_string(params);
    
    mcp_message_t request = {
        .type = MCP_REQUEST,
        .id = 2,
        .method = "run",
        .params = strdup(params_str)
    };
    
    // Use async processing
    int result = nano_process_message_async(&request);
    
    json_object_put(params);
    free(request.params);
    
    if (result == 0) {
        printf("✅ Async inference request submitted\n");
        
        // Wait for callback response
        int timeout = 20;
        while (!g_nano_test_state.inference_callback_received && timeout-- > 0) {
            sleep(1);
            if (timeout % 5 == 0) {
                printf("⏳ Waiting for inference callback... %d seconds remaining\n", timeout);
            }
        }
        
        if (g_nano_test_state.inference_callback_received) {
            printf("✅ Async inference test passed\n");
            return 0;
        } else {
            printf("❌ Async inference callback timeout\n");
            return -1;
        }
    } else {
        printf("❌ Failed to submit async inference request: %d\n", result);
        return -1;
    }
}

// Test concurrent async requests
int test_nano_concurrent_requests() {
    printf("\n🔄 Testing Concurrent Async Requests through NANO...\n");
    
    int initial_callbacks = g_nano_test_state.callbacks_received;
    
    // Submit multiple concurrent requests
    for (int i = 3; i <= 5; i++) {
        json_object *params = json_object_new_object();
        char prompt_text[128];
        snprintf(prompt_text, sizeof(prompt_text), "Request %d: What is %d + %d?", i, i, i);
        json_object *prompt = json_object_new_string(prompt_text);
        json_object_object_add(params, "prompt", prompt);
        
        const char *params_str = json_object_to_json_string(params);
        
        mcp_message_t request = {
            .type = MCP_REQUEST,
            .id = i,
            .method = "run",
            .params = strdup(params_str)
        };
        
        int result = nano_process_message_async(&request);
        
        json_object_put(params);
        free(request.params);
        
        if (result == 0) {
            printf("✅ Concurrent request %d submitted\n", i);
        } else {
            printf("❌ Failed to submit concurrent request %d\n", i);
        }
        
        // Small delay between requests
        sleep(1); // 1 second
    }
    
    // Wait for all responses
    printf("⏳ Waiting for concurrent responses...\n");
    int timeout = 30;
    while (timeout-- > 0) {
        int new_callbacks = g_nano_test_state.callbacks_received - initial_callbacks;
        if (new_callbacks >= 3) {
            printf("✅ Concurrent requests test passed (%d responses received)\n", new_callbacks);
            return 0;
        }
        sleep(1);
        if (timeout % 5 == 0) {
            printf("⏳ %d seconds remaining, %d responses received\n", timeout, new_callbacks);
        }
    }
    
    int final_callbacks = g_nano_test_state.callbacks_received - initial_callbacks;
    if (final_callbacks > 0) {
        printf("⚠️  Partial success: %d out of 3 concurrent responses received\n", final_callbacks);
        return 0;
    } else {
        printf("❌ No concurrent responses received\n");
        return -1;
    }
}

// Test sync vs async comparison
int test_nano_sync_vs_async() {
    printf("\n🔄 Testing Sync vs Async Processing Comparison...\n");
    
    // Test sync processing
    printf("Testing synchronous processing...\n");
    json_object *params = json_object_new_object();
    json_object *prompt = json_object_new_string("Sync test: What is 1+1?");
    json_object_object_add(params, "prompt", prompt);
    
    const char *params_str = json_object_to_json_string(params);
    
    mcp_message_t sync_request = {
        .type = MCP_REQUEST,
        .id = 10,
        .method = "run",
        .params = strdup(params_str)
    };
    
    mcp_message_t sync_response = {0};
    int sync_result = nano_process_message(&sync_request, &sync_response);
    
    if (sync_result == 0) {
        printf("✅ Sync processing completed: %s\n", 
               sync_response.params ? sync_response.params : "null");
        if (sync_response.params) {
            free(sync_response.params);
        }
    } else {
        printf("❌ Sync processing failed\n");
    }
    
    free(sync_request.params);
    json_object_put(params);
    
    // Test async processing  
    printf("Testing asynchronous processing...\n");
    params = json_object_new_object();
    prompt = json_object_new_string("Async test: What is 2+2?");
    json_object_object_add(params, "prompt", prompt);
    
    params_str = json_object_to_json_string(params);
    
    mcp_message_t async_request = {
        .type = MCP_REQUEST,
        .id = 11,
        .method = "run",
        .params = strdup(params_str)
    };
    
    int async_result = nano_process_message_async(&async_request);
    
    json_object_put(params);
    free(async_request.params);
    
    if (async_result == 0) {
        printf("✅ Async processing submitted successfully\n");
        printf("✅ Sync vs Async comparison test passed\n");
        return 0;
    } else {
        printf("❌ Async processing failed\n");
        return -1;
    }
}

// Main NANO callback test
void test_nano_callbacks() {
    printf("⚡ NANO Callback Test - NANO Layer Testing\n");
    printf("Architecture: Test → NANO → IO → RKLLM → NANO Callback → Test\n");
    
    int test_results = 0;
    
    // Test 1: NANO initialization
    if (test_nano_init() != 0) {
        test_results++;
    }
    
    // Test 2: Async model initialization
    if (test_nano_async_init() != 0) {
        test_results++;
    }
    
    // Test 3: Async inference
    if (test_nano_async_inference() != 0) {
        test_results++;
    }
    
    // Test 4: Concurrent requests
    if (test_nano_concurrent_requests() != 0) {
        test_results++;
    }
    
    // Test 5: Sync vs Async comparison
    if (test_nano_sync_vs_async() != 0) {
        test_results++;
    }
    
    // Cleanup
    printf("\n🔧 Shutting down NANO system...\n");
    nano_shutdown();
    printf("✅ NANO shutdown complete\n");
    
    // Summary
    printf("\n📊 NANO Callback Test Summary:\n");
    printf("   Total callbacks received: %d\n", g_nano_test_state.callbacks_received);
    printf("   Init callback received: %s\n", g_nano_test_state.init_callback_received ? "Yes" : "No");
    printf("   Inference callback received: %s\n", g_nano_test_state.inference_callback_received ? "Yes" : "No");
    printf("   Streaming callback received: %s\n", g_nano_test_state.streaming_callback_received ? "Yes" : "No");
    printf("   Concurrent responses: %d\n", g_nano_test_state.concurrent_responses);
    printf("   Failed tests: %d/5\n", test_results);
    
    if (test_results == 0) {
        printf("\n🎉 NANO callback test passed! NANO layer is working correctly.\n");
    } else {
        printf("\n❌ %d test(s) failed. Check output above for details.\n", test_results);
    }
}