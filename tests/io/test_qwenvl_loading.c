#include "test_io.h"
#include <sys/stat.h>
#include <unistd.h>

int test_qwenvl_full(void) {
    printf("🚀 Test 1: QwenVL (load → inference → cleanup)\n");
    
    // Force memory cleanup before starting
    system_force_gc();
    
    uint32_t handle = 0;
    struct stat st;
    
    // Check model file exists
    if (stat("models/qwenvl/model.rkllm", &st) != 0) {
        printf("❌ QwenVL model not found\n");
        return 1;
    }
    
    printf("📁 QwenVL model: %zu MB\n", st.st_size / (1024 * 1024));
    printf("🔄 Loading QwenVL model...\n");
    
    // Load model
    char request[1024];
    snprintf(request, sizeof(request),
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"init\",\"params\":{\"model_path\":\"models/qwenvl/model.rkllm\",\"max_context_len\":512,\"temperature\":0.7}}");
    
    if (io_push_request(request) != 0) {
        printf("❌ Failed to push QwenVL init request\n");
        return 1;
    }
    
    // Wait for model to load with extended timeout for large models
    char response[1024];
    for (int i = 0; i < 300; i++) {  // Increased timeout for large model
        if (io_pop_response(response, sizeof(response)) == 0) {
            if (strstr(response, "\"handle_id\"")) {
                const char* handle_start = strstr(response, "\"handle_id\":");
                if (handle_start && sscanf(handle_start + 12, "%u", &handle) == 1) {
                    printf("✅ QwenVL loaded successfully (handle=%u)\n", handle);
                    break;
                }
            } else if (strstr(response, "\"error\"")) {
                printf("❌ QwenVL init error: %s\n", response);
                return 1;
            }
        }
        
        // Print progress every 10 seconds
        if (i % 100 == 0 && i > 0) {
            printf("⏳ Still loading... (%d seconds)\n", i / 10);
        }
        
        usleep(100000);
    }
    
    if (handle == 0) {
        printf("❌ QwenVL init timeout (model too large or system under load)\n");
        printf("🔍 This may indicate insufficient memory or system resources\n");
        return 1;
    }
    
    // Give the model some time to fully initialize
    printf("⏳ Waiting for model to stabilize...\n");
    sleep(2);
    
    // Test inference
    printf("🧪 Testing QwenVL inference:\n");
    int inference_success = test_model_inference(handle, "QwenVL");
    
    // Cleanup
    test_cleanup_model(handle, "QwenVL");
    
    printf("📊 QwenVL test: %s\n", inference_success ? "SUCCESS" : "FAILED");
    return inference_success ? 0 : 1;
}
