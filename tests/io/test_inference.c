#include "test_io.h"

int test_qwenvl_inference(uint32_t handle) {
    if (handle == 0) {
        printf("❌ QwenVL handle invalid, skipping inference test\n");
        return 1;
    }
    
    printf("\n🤖 Testing QwenVL inference with real prompt...\n");
    char run_request[1024];
    snprintf(run_request, sizeof(run_request),
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"run\",\"params\":{\"handle_id\":%u,\"prompt\":\"Xin chào, bạn có khỏe không?\"}}", handle);
    
    printf("Test 4a: QwenVL inference... ");
    if (io_push_request(run_request) != 0) {
        printf("FAIL - push failed\n");
        return 1;
    }
    
    char response[1024];
    int retries = 100;
    while (retries-- > 0) {
        if (io_pop_response(response, sizeof(response)) == 0) {
            printf("Response: %s\n", response);
            if (strstr(response, "\"text\"")) {
                printf("✅ SUCCESS - QwenVL responded!\n");
                return 0;
            } else if (strstr(response, "\"error\"")) {
                printf("❌ FAILED - QwenVL inference error: %s\n", response);
                return 1;
            }
        }
        usleep(100000);
    }
    
    printf("❌ FAILED - No response from QwenVL inference\n");
    return 1;
}

int test_lora_inference(uint32_t handle) {
    if (handle == 0) {
        printf("❌ LoRA handle invalid, skipping inference test\n");
        return 1;
    }
    
    printf("\n🤖 Testing LoRA inference with real prompt...\n");
    char lora_run[1024];
    snprintf(lora_run, sizeof(lora_run),
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"run\",\"params\":{\"handle_id\":%u,\"prompt\":\"Hi\"}}", handle);
    
    printf("Test 4b: LoRA inference... ");
    if (io_push_request(lora_run) != 0) {
        printf("FAIL - push failed\n");
        return 1;
    }
    
    char response[1024];
    int retries = 100;
    while (retries-- > 0) {
        if (io_pop_response(response, sizeof(response)) == 0) {
            printf("Response: %s\n", response);
            if (strstr(response, "\"text\"")) {
                printf("✅ SUCCESS - LoRA responded!\n");
                return 0;
            } else if (strstr(response, "\"error\"")) {
                printf("❌ FAILED - LoRA inference error: %s\n", response);
                return 1;
            }
        }
        usleep(100000);
    }
    
    printf("❌ FAILED - No response from LoRA inference\n");
    return 1;
}
