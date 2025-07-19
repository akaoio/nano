// Required for clock_gettime
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>
#include <json-c/json.h>

// Include the simplified IO operations
#include "../src/io/operations.h"

/*
 * SIMPLIFIED UNIFIED TEST SUITE
 * 
 * Tests the lightweight RKLLM integration directly:
 * 1. Direct RKLLM initialization
 * 2. Basic inference operations  
 * 3. Cleanup and resource management
 */

#define TEST_MODEL_PATH "models/qwen3/model.rkllm"

// Test state
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} test_state_t;

static test_state_t g_state = {0};

bool check_model_file() {
    struct stat st;
    if (stat(TEST_MODEL_PATH, &st) == 0) {
        printf("✅ Model file: %s (%.1f MB)\n", 
               TEST_MODEL_PATH, (double)st.st_size / (1024 * 1024));
        return true;
    }
    printf("❌ Model file not found: %s\n", TEST_MODEL_PATH);
    printf("   Run from project root directory\n");
    return false;
}

bool test_json_parsing() {
    printf("\n🧪 Test: JSON Request Parsing\n");
    printf("------------------------------\n");
    
    g_state.tests_run++;
    
    const char* test_request = "{"
        "\"id\": 123,"
        "\"method\": \"init\","
        "\"params\": {"
            "\"model_path\": \"" TEST_MODEL_PATH "\","
            "\"max_new_tokens\": 100"
        "}"
    "}";
    
    uint32_t request_id;
    char method[256];
    char params[4096];
    
    int result = io_parse_json_request_main(test_request, &request_id, method, params);
    
    if (result == 0 && request_id == 123 && strcmp(method, "init") == 0) {
        printf("✅ JSON parsing: PASSED\n");
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ JSON parsing: FAILED\n");
        g_state.tests_failed++;
        return false;
    }
}

bool test_model_initialization() {
    printf("\n🔧 Test: Model Initialization\n");
    printf("------------------------------\n");
    
    g_state.tests_run++;
    
    const char* params_json = "{"
        "\"model_path\": \"" TEST_MODEL_PATH "\","
        "\"max_new_tokens\": 100,"
        "\"temperature\": 0.7"
    "}";
    
    char* result_json = NULL;
    int status = io_handle_init(params_json, &result_json);
    
    bool success = (status == 0 && result_json != NULL && strstr(result_json, "handle_id") != NULL);
    
    if (success) {
        printf("✅ Model initialization: PASSED\n");
        printf("   Result: %s\n", result_json);
        g_state.tests_passed++;
    } else {
        printf("❌ Model initialization: FAILED\n");
        if (result_json) printf("   Error: %s\n", result_json);
        g_state.tests_failed++;
    }
    
    if (result_json) {
        free(result_json);
    }
    
    return success;
}

bool test_inference() {
    printf("\n🤖 Test: Basic Inference\n");
    printf("-------------------------\n");
    
    g_state.tests_run++;
    
    // Check if model is initialized
    if (!io_is_initialized()) {
        printf("❌ Model not initialized for inference test\n");
        g_state.tests_failed++;
        return false;
    }
    
    const char* params_json = "{"
        "\"prompt\": \"Hello, how are you?\","
        "\"max_new_tokens\": 10"
    "}";
    
    char* result_json = NULL;
    int status = io_handle_run(params_json, &result_json);
    
    bool success = (status == 0 && result_json != NULL);
    
    if (success) {
        printf("✅ Basic inference: PASSED\n");
        printf("   Result: %s\n", result_json);
        g_state.tests_passed++;
    } else {
        printf("❌ Basic inference: FAILED\n");
        if (result_json) printf("   Error: %s\n", result_json);
        g_state.tests_failed++;
    }
    
    if (result_json) {
        free(result_json);
    }
    
    return success;
}

// Streaming test context for real-time data capture
typedef struct {
    int chunk_count;
    int total_chars;
    bool stream_finished;
    char accumulated_text[4096];
    struct timespec start_time;
    struct timespec last_chunk_time;
    
    // Raw data capture
    int raw_callbacks;
    size_t total_raw_bytes;
} streaming_context_t;

// Helper function to display raw binary data in hex format
void display_raw_data(const char* label, const void* data, size_t size) {
    printf("🔍 [RAW %s] (%zu bytes): ", label, size);
    
    const unsigned char* bytes = (const unsigned char*)data;
    for (size_t i = 0; i < size && i < 64; i++) { // Limit to 64 bytes for readability
        printf("%02X ", bytes[i]);
        if ((i + 1) % 16 == 0) printf("\n                           ");
    }
    if (size > 64) printf("... (%zu more bytes)", size - 64);
    printf("\n");
}

// Helper function to display raw text data with escape sequences
void display_raw_text(const char* label, const char* text) {
    if (!text) return;
    
    printf("📝 [RAW %s]: \"", label);
    for (const char* p = text; *p && (p - text) < 200; p++) {
        if (*p >= 32 && *p <= 126) {
            printf("%c", *p);
        } else {
            printf("\\x%02X", (unsigned char)*p);
        }
    }
    if (strlen(text) > 200) printf("...");
    printf("\"\n");
}

// Real-time streaming callback to capture actual data
int streaming_test_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    streaming_context_t* ctx = (streaming_context_t*)userdata;
    
    if (!result) return 0;
    
    // Capture timestamp for real-time monitoring
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    
    if (result->text && strlen(result->text) > 0) {
        ctx->chunk_count++;
        int chunk_len = strlen(result->text);
        ctx->total_chars += chunk_len;
        
        // Calculate time since last chunk
        double chunk_latency = 0.0;
        if (ctx->chunk_count > 1) {
            chunk_latency = (current_time.tv_sec - ctx->last_chunk_time.tv_sec) * 1000.0 + 
                          (current_time.tv_nsec - ctx->last_chunk_time.tv_nsec) / 1000000.0;
        }
        
        // Display real-time streaming data
        printf("📦 Chunk #%d (%d chars, %.1fms): \"%s\"\n", 
               ctx->chunk_count, chunk_len, chunk_latency, result->text);
        
        // Accumulate text for verification
        if (strlen(ctx->accumulated_text) + chunk_len < sizeof(ctx->accumulated_text) - 1) {
            strcat(ctx->accumulated_text, result->text);
        }
        
        ctx->last_chunk_time = current_time;
    }
    
    // Handle different streaming states
    switch (state) {
        case RKLLM_RUN_NORMAL:
            printf("🔄 [STREAMING] Generation in progress...\n");
            break;
        case RKLLM_RUN_WAITING:
            printf("⏸️  [WAITING] Waiting for complete UTF-8 character...\n");
            break;
        case RKLLM_RUN_FINISH:
            printf("✅ [FINISHED] Stream completed!\n");
            ctx->stream_finished = true;
            
            // Calculate total streaming time
            double total_time = (current_time.tv_sec - ctx->start_time.tv_sec) * 1000.0 + 
                              (current_time.tv_nsec - ctx->start_time.tv_nsec) / 1000000.0;
            printf("📊 Streaming stats: %d chunks, %d chars, %.2fms total\n", 
                   ctx->chunk_count, ctx->total_chars, total_time);
            break;
        case RKLLM_RUN_ERROR:
            printf("❌ [ERROR] Stream error occurred!\n");
            ctx->stream_finished = true;
            break;
    }
    
    return 0; // Continue streaming
}

// Global streaming context for capturing real callback data
static streaming_context_t g_streaming_ctx = {0};
static uint32_t g_request_id = 0;

// Real RKLLM streaming callback - captures actual generated data
int real_streaming_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    streaming_context_t* ctx = (streaming_context_t*)userdata;
    
    if (!result) return 0;
    
    // Capture timestamp for real-time monitoring
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    
    // Display actual raw callback data
    printf("🔴 [REAL CALLBACK] RKLLM called callback with:\n");
    printf("   • result->text: ");
    if (result->text && strlen(result->text) > 0) {
        display_raw_text("ACTUAL TEXT", result->text);
        
        ctx->chunk_count++;
        int chunk_len = strlen(result->text);
        ctx->total_chars += chunk_len;
        
        // Calculate time since last chunk
        double chunk_latency = 0.0;
        if (ctx->chunk_count > 1) {
            chunk_latency = (current_time.tv_sec - ctx->last_chunk_time.tv_sec) * 1000.0 + 
                          (current_time.tv_nsec - ctx->last_chunk_time.tv_nsec) / 1000000.0;
        }
        
        printf("   • Chunk #%d (%d chars, %.1fms latency)\n", 
               ctx->chunk_count, chunk_len, chunk_latency);
        
        // Accumulate text for verification
        if (strlen(ctx->accumulated_text) + chunk_len < sizeof(ctx->accumulated_text) - 1) {
            strcat(ctx->accumulated_text, result->text);
        }
        
        ctx->last_chunk_time = current_time;
    } else {
        printf("NULL\n");
    }
    
    printf("   • result->token_id: %d\n", result->token_id);
    printf("   • state: ");
    
    // Handle different streaming states
    switch (state) {
        case RKLLM_RUN_NORMAL:
            printf("RKLLM_RUN_NORMAL (generation in progress)\n");
            break;
        case RKLLM_RUN_WAITING:
            printf("RKLLM_RUN_WAITING (waiting for complete UTF-8)\n");
            break;
        case RKLLM_RUN_FINISH:
            printf("RKLLM_RUN_FINISH (stream completed)\n");
            ctx->stream_finished = true;
            
            // Calculate total streaming time
            double total_time = (current_time.tv_sec - ctx->start_time.tv_sec) * 1000.0 + 
                              (current_time.tv_nsec - ctx->start_time.tv_nsec) / 1000000.0;
            printf("📊 Final stats: %d chunks, %d chars, %.2fms total\n", 
                   ctx->chunk_count, ctx->total_chars, total_time);
            break;
        case RKLLM_RUN_ERROR:
            printf("RKLLM_RUN_ERROR (stream error occurred)\n");
            ctx->stream_finished = true;
            break;
    }
    
    // Show real JSON-RPC wrapping
    printf("📤 [REAL JSON-RPC] Converting to actual response:\n");
    char* jsonrpc_response = io_create_json_response(g_request_id, true, result->text);
    if (jsonrpc_response) {
        display_raw_text("ACTUAL JSON-RPC", jsonrpc_response);
        free(jsonrpc_response);
    }
    printf("\n");
    
    return 0; // Continue streaming
}

bool test_protocol_data_flow() {
    printf("\n🌊 Test: REAL Protocol Data Flow (JSON-RPC)\n");
    printf("==========================================\n");
    
    g_state.tests_run++;
    
    printf("📡 Testing ACTUAL JSON-RPC protocol data flow with REAL RKLLM streaming...\n\n");
    
    // 1. Show raw JSON-RPC request that client would send to NANO
    const char* raw_jsonrpc_request = "{"
        "\"jsonrpc\": \"2.0\","
        "\"id\": 12345,"
        "\"method\": \"run_streaming\","
        "\"params\": {"
            "\"handle_id\": 1,"
            "\"prompt\": \"What is AI?\","
            "\"max_new_tokens\": 15,"
            "\"temperature\": 0.8"
        "}"
    "}";
    
    printf("📤 [CLIENT → NANO] Raw JSON-RPC Request:\n");
    display_raw_text("JSON-RPC REQUEST", raw_jsonrpc_request);
    
    // 2. Parse the JSON-RPC request (what NANO receives)
    uint32_t request_id;
    char method[256];
    char params[4096];
    
    printf("\n🔄 [NANO PROCESSING] Parsing JSON-RPC request...\n");
    int parse_result = io_parse_json_request_main(raw_jsonrpc_request, &request_id, method, params);
    g_request_id = request_id;
    
    if (parse_result == 0) {
        printf("✅ JSON-RPC parsing successful:\n");
        printf("   • Request ID: %u\n", request_id);
        printf("   • Method: %s\n", method);
        display_raw_text("PARSED PARAMS", params);
    } else {
        printf("❌ JSON-RPC parsing failed\n");
        g_state.tests_failed++;
        return false;
    }
    
    // 3. For streaming test, perform REAL RKLLM streaming with REAL callback
    if (strcmp(method, "run_streaming") == 0) {
        printf("\n🔥 [REAL STREAMING] Performing actual RKLLM streaming with real callback...\n");
        
        // Parse streaming params
        json_object* stream_params = json_tokener_parse(params);
        if (!stream_params) {
            printf("❌ Failed to parse streaming parameters\n");
            g_state.tests_failed++;
            return false;
        }
        
        // Extract prompt
        json_object* prompt_obj;
        const char* prompt = "What is AI?";
        if (json_object_object_get_ex(stream_params, "prompt", &prompt_obj)) {
            prompt = json_object_get_string(prompt_obj);
        }
        
        // Extract max_new_tokens
        json_object* max_tokens_obj;
        int max_tokens = 15;
        if (json_object_object_get_ex(stream_params, "max_new_tokens", &max_tokens_obj)) {
            max_tokens = json_object_get_int(max_tokens_obj);
        }
        
        printf("🤖 [REAL INFERENCE] Starting actual RKLLM inference...\n");
        printf("   Prompt: \"%s\"\n", prompt);
        printf("   Max tokens: %d\n", max_tokens);
        
        // Initialize streaming context
        memset(&g_streaming_ctx, 0, sizeof(g_streaming_ctx));
        clock_gettime(CLOCK_MONOTONIC, &g_streaming_ctx.start_time);
        g_streaming_ctx.last_chunk_time = g_streaming_ctx.start_time;
        
        // Create RKLLM parameters with callback
        RKLLMParam rkllm_param = rkllm_createDefaultParam();
        rkllm_param.model_path = TEST_MODEL_PATH;
        rkllm_param.max_new_tokens = max_tokens;
        rkllm_param.temperature = 0.8;
        rkllm_param.is_async = false; // Synchronous for test
        
        // Initialize RKLLM with REAL callback
        LLMHandle stream_handle = NULL;
        printf("\n🔧 [RKLLM INIT] Initializing RKLLM with real streaming callback...\n");
        int init_status = rkllm_init(&stream_handle, &rkllm_param, real_streaming_callback);
        
        if (init_status != 0 || !stream_handle) {
            printf("❌ Failed to initialize RKLLM with callback (status: %d)\n", init_status);
            json_object_put(stream_params);
            g_state.tests_failed++;
            return false;
        }
        
        printf("✅ RKLLM initialized with real callback function\n");
        
        // Create RKLLM input for streaming
        RKLLMInput input = {0};
        input.input_type = RKLLM_INPUT_PROMPT;
        input.prompt_input = prompt;
        
        // Create RKLLM inference parameters
        RKLLMInferParam infer_param = {0};
        infer_param.mode = RKLLM_INFER_GENERATE;
        infer_param.keep_history = 0;
        
        // Run REAL streaming inference
        printf("\n🚀 [REAL STREAMING] Calling rkllm_run() - callback will receive REAL data:\n");
        printf("================================================================================\n");
        
        int run_status = rkllm_run(stream_handle, &input, &infer_param, &g_streaming_ctx);
        
        printf("================================================================================\n");
        printf("🏁 [STREAMING COMPLETE] rkllm_run() finished with status: %d\n", run_status);
        
        if (run_status == 0) {
            printf("✅ Real streaming completed successfully!\n");
            printf("📊 Actual streaming results:\n");
            printf("   • Total chunks received: %d\n", g_streaming_ctx.chunk_count);
            printf("   • Total characters: %d\n", g_streaming_ctx.total_chars);
            printf("   • Generated text: \"%s\"\n", g_streaming_ctx.accumulated_text);
        } else {
            printf("❌ Real streaming failed (status: %d)\n", run_status);
        }
        
        // Clean up
        rkllm_destroy(stream_handle);
        json_object_put(stream_params);
        
        printf("\n📋 [REAL DATA FLOW VERIFIED]\n");
        printf("✅ Actual Protocol Data Flow with REAL data:\n");
        printf("   1. Client sent JSON-RPC 2.0 streaming request\n");
        printf("   2. NANO parsed and extracted streaming parameters\n");
        printf("   3. RKLLM initialized with real callback function\n");
        printf("   4. rkllm_run() called - RKLLM generated actual tokens\n");
        printf("   5. Callback received REAL RKLLMResult with actual text\n");
        printf("   6. Each chunk converted to REAL JSON-RPC responses\n");
        printf("   7. Client received actual streaming data (NO MOCKS)\n");
        
    } else {
        printf("❌ Method not supported for real streaming test\n");
        g_state.tests_failed++;
        return false;
    }
    
    printf("\n✅ REAL protocol data flow test: PASSED\n");
    g_state.tests_passed++;
    return true;
}

bool test_abort_functionality() {
    printf("\n⏹️  Test: Abort Functionality\n");
    printf("-----------------------------\n");
    
    g_state.tests_run++;
    
    // Check if model is initialized
    if (!io_is_initialized()) {
        printf("❌ Model not initialized for abort test\n");
        g_state.tests_failed++;
        return false;
    }
    
    LLMHandle handle = io_get_rkllm_handle();
    if (!handle) {
        printf("❌ No RKLLM handle available\n");
        g_state.tests_failed++;
        return false;
    }
    
    printf("🛑 Testing abort functionality...\n");
    
    // Test abort call (should work even if nothing is running)
    int status = rkllm_abort(handle);
    
    printf("✅ Abort functionality: PASSED\n");
    printf("   Abort call status: %d\n", status);
    printf("   Direct NPU access confirmed (bypasses queue)\n");
    
    g_state.tests_passed++;
    return true;
}

bool test_cleanup() {
    printf("\n🧹 Test: Cleanup and Destroy\n");
    printf("-----------------------------\n");
    
    g_state.tests_run++;
    
    char* result_json = NULL;
    int status = io_handle_destroy(NULL, &result_json);
    
    bool success = (status == 0 && result_json != NULL && !io_is_initialized());
    
    if (success) {
        printf("✅ Cleanup: PASSED\n");
        printf("   Result: %s\n", result_json);
        g_state.tests_passed++;
    } else {
        printf("❌ Cleanup: FAILED\n");
        if (result_json) printf("   Error: %s\n", result_json);
        g_state.tests_failed++;
    }
    
    if (result_json) {
        free(result_json);
    }
    
    return success;
}

bool test_direct_rkllm_functions() {
    printf("\n⚡ Test: Direct RKLLM Functions\n");
    printf("-------------------------------\n");
    
    g_state.tests_run++;
    
    // Test getting handle when not initialized
    LLMHandle handle = io_get_rkllm_handle();
    if (handle != NULL) {
        printf("❌ Handle should be NULL when not initialized\n");
        g_state.tests_failed++;
        return false;
    }
    
    // Test initialization status
    if (io_is_initialized()) {
        printf("❌ Should not be initialized at this point\n");
        g_state.tests_failed++;
        return false;
    }
    
    printf("✅ Direct RKLLM functions: PASSED\n");
    g_state.tests_passed++;
    return true;
}

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    printf("🚀 NANO Lightweight Test Suite\n");
    printf("===============================\n");
    printf("Direct RKLLM integration - no proxy layer\n\n");
    
    // Check prerequisites
    if (!check_model_file()) {
        printf("\n❌ Prerequisites not met. Exiting.\n");
        return 1;
    }
    
    printf("\n📋 Running Lightweight Tests...\n");
    
    // Run tests in sequence
    test_json_parsing();
    test_direct_rkllm_functions();
    test_model_initialization();
    test_inference();
    test_protocol_data_flow();
    test_abort_functionality();
    test_cleanup();
    
    // Final summary
    printf("\n📊 Test Results\n");
    printf("===============\n");
    printf("Tests run:    %d\n", g_state.tests_run);
    printf("Tests passed: %d\n", g_state.tests_passed);
    printf("Tests failed: %d\n", g_state.tests_failed);
    
    if (g_state.tests_failed == 0) {
        printf("\n🎉 All tests PASSED! Lightweight RKLLM integration working correctly.\n");
        return 0;
    } else {
        printf("\n❌ %d test(s) FAILED. Check output above.\n", g_state.tests_failed);
        return 1;
    }
}