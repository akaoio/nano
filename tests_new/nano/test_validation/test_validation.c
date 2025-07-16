#include "validation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Create a test model file with version info
int create_test_model(const char* filename, const char* version_pattern) {
    FILE* file = fopen(filename, "wb");
    if (!file) return -1;
    
    // Write some dummy data with version pattern
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "MODEL_HEADER%sEND", version_pattern);
    fwrite(buffer, 1, strlen(buffer), file);
    fclose(file);
    return 0;
}

void test_model_checker_init() {
    printf("Testing model_checker_init...\n");
    
    int result = model_checker_init();
    assert(result == 0);
    
    model_checker_shutdown();
    
    printf("model_checker_init tests passed!\n");
}

void test_model_check_version() {
    printf("Testing model_check_version...\n");
    
    // Create test model file
    create_test_model("test_model.tmp", "1.2.1");
    
    model_version_info_t version;
    int result = model_check_version("test_model.tmp", &version);
    assert(result == 0);
    assert(version.major == 1);
    assert(version.minor == 2);
    assert(version.patch == 1);
    assert(strcmp(version.version_string, "1.2.1") == 0);
    
    // Test with non-existent file
    result = model_check_version("non_existent.tmp", &version);
    assert(result == -1);
    
    // Cleanup
    remove("test_model.tmp");
    
    printf("model_check_version tests passed!\n");
}

void test_model_check_compatibility() {
    printf("Testing model_check_compatibility...\n");
    
    // Create compatible model
    create_test_model("compatible_model.tmp", "1.2.1");
    
    compatibility_result_t result;
    int ret = model_check_compatibility("compatible_model.tmp", &result);
    assert(ret == 0);
    assert(result.is_compatible == true);
    assert(strlen(result.error_message) == 0);
    
    // Create incompatible model
    create_test_model("incompatible_model.tmp", "1.1.1");
    
    ret = model_check_compatibility("incompatible_model.tmp", &result);
    assert(ret == -1);
    assert(result.is_compatible == false);
    assert(strlen(result.error_message) > 0);
    
    // Cleanup
    remove("compatible_model.tmp");
    remove("incompatible_model.tmp");
    
    printf("model_check_compatibility tests passed!\n");
}

void test_model_check_lora_compatibility() {
    printf("Testing model_check_lora_compatibility...\n");
    
    // Create compatible base model and LoRA
    create_test_model("base_model.tmp", "1.2.1");
    create_test_model("lora_adapter.tmp", "1.2.1");
    
    compatibility_result_t result;
    int ret = model_check_lora_compatibility("base_model.tmp", "lora_adapter.tmp", &result);
    assert(ret == 0);
    assert(result.is_compatible == true);
    
    // Create incompatible LoRA
    create_test_model("incompatible_lora.tmp", "1.1.1");
    
    ret = model_check_lora_compatibility("base_model.tmp", "incompatible_lora.tmp", &result);
    assert(ret == -1);
    assert(result.is_compatible == false);
    assert(strlen(result.error_message) > 0);
    
    // Cleanup
    remove("base_model.tmp");
    remove("lora_adapter.tmp");
    remove("incompatible_lora.tmp");
    
    printf("model_check_lora_compatibility tests passed!\n");
}

void test_model_get_runtime_version() {
    printf("Testing model_get_runtime_version...\n");
    
    const char* version = model_get_runtime_version();
    assert(version != NULL);
    assert(strcmp(version, "1.2.1") == 0);
    
    printf("model_get_runtime_version tests passed!\n");
}

int main() {
    printf("Running validation tests...\n");
    
    test_model_checker_init();
    test_model_check_version();
    test_model_check_compatibility();
    test_model_check_lora_compatibility();
    test_model_get_runtime_version();
    
    printf("All validation tests passed!\n");
    return 0;
}
