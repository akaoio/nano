#include "test_io.h"

int test_lora_loading(uint32_t* handle) {
    *handle = 0;
    
    FILE *lora_base_file = fopen("models/lora/model.rkllm", "r");
    if (!lora_base_file) {
        printf("❌ LoRA base model not found, skipping test\n");
        return 1;
    }
    fclose(lora_base_file);
    
    char lora_init[1024];
    snprintf(lora_init, sizeof(lora_init),
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"init\",\"params\":{\"model_path\":\"models/lora/model.rkllm\",\"max_context_len\":256,\"temperature\":0.8}}");
    
    printf("Test 3b: LoRA init... ");
    if (io_push_request(lora_init) != 0) {
        printf("FAIL - push failed\n");
        return 1;
    }
    
    char response[1024];
    int retries = 50;
    while (retries-- > 0) {
        if (io_pop_response(response, sizeof(response)) == 0) {
            printf("Response: %s\n", response);
            if (strstr(response, "\"handle_id\"")) {
                sscanf(response, "{\"handle_id\":%u}", handle);
                printf("✅ SUCCESS - LoRA loaded, handle=%u\n", *handle);
                return 0;
            } else if (strstr(response, "\"error\"")) {
                printf("❌ FAILED - LoRA loading error: %s\n", response);
                return 1;
            }
        }
        usleep(100000);
    }
    
    printf("❌ FAILED - No response from LoRA init\n");
    return 1;
}
