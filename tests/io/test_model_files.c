#include "test_io.h"

int test_model_files(void) {
    printf("\n📁 Checking model files...\n");
    
    FILE *qwenvl_file = fopen("models/qwenvl/model.rkllm", "r");
    long qwenvl_size = 0;
    if (qwenvl_file) {
        fseek(qwenvl_file, 0, SEEK_END);
        qwenvl_size = ftell(qwenvl_file);
        fclose(qwenvl_file);
        printf("✅ QwenVL model found: %ld bytes\n", qwenvl_size);
    } else {
        printf("❌ QwenVL model not found: models/qwenvl/model.rkllm\n");
    }
    
    FILE *lora_base_file = fopen("models/lora/model.rkllm", "r");
    long lora_base_size = 0;
    if (lora_base_file) {
        fseek(lora_base_file, 0, SEEK_END);
        lora_base_size = ftell(lora_base_file);
        fclose(lora_base_file);
        printf("✅ LoRA base model found: %ld bytes\n", lora_base_size);
    } else {
        printf("❌ LoRA base model not found: models/lora/model.rkllm\n");
    }
    
    FILE *lora_adapter_file = fopen("models/lora/lora.rkllm", "r");
    long lora_adapter_size = 0;
    if (lora_adapter_file) {
        fseek(lora_adapter_file, 0, SEEK_END);
        lora_adapter_size = ftell(lora_adapter_file);
        fclose(lora_adapter_file);
        printf("✅ LoRA adapter found: %ld bytes\n", lora_adapter_size);
    } else {
        printf("❌ LoRA adapter not found: models/lora/lora.rkllm\n");
    }
    
    return 0;
}
