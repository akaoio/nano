#include "../test_qwenvl.h"
#include "../../../src/io/mapping/handle_pool/handle_pool.h"
#include "../../../src/io/mapping/rkllm_proxy/rkllm_proxy.h"
#include <json-c/json.h>
#include "../../../src/nano/validation/model_checker/model_checker.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Forward declaration
int test_qwenvl_inference(uint32_t handle_id);

// Test with real QwenVL model
int test_qwenvl_model_loading() {
    printf("🔍 Testing QwenVL model loading...\n");
    
    const char* model_path = "/home/x/Projects/nano/models/qwenvl/model.rkllm";
    
    // Step 1: Pre-flight model validation
    printf("🔍 Step 1: Pre-flight model validation...\n");
    compatibility_result_t compat_result;
    if (model_check_compatibility(model_path, &compat_result) != 0) {
        printf("❌ Model compatibility check failed: %s\n", compat_result.error_message);
        return 1;
    }
    
    if (!compat_result.is_compatible) {
        printf("❌ Model is not compatible: %s\n", compat_result.error_message);
        return 1;
    }
    
    printf("✅ Model compatibility check passed\n");
    printf("📊 Model version: %s\n", compat_result.model_info.version_string);
    
    // Step 2: Initialize systems
    printf("🔍 Step 2: Initialize systems...\n");
    printf("Debug: Initializing systems...\n");
    int init_result = rkllm_proxy_init();
    if (init_result != 0) {
        printf("❌ Failed to initialize proxy system\n");
        return 1;
    }
    printf("Debug: Proxy system initialized\n");
    
    // Step 3: Load model using json-c
    printf("🔍 Step 3: Loading model...\n");
    printf("Debug: Preparing model loading params...\n");
    
    json_object *params_obj = json_object_new_object();
    json_object *model_path_obj = json_object_new_string(model_path);
    json_object_object_add(params_obj, "model_path", model_path_obj);
    
    const char *params_json = json_object_to_json_string(params_obj);
    printf("Debug: Params: %s\n", params_json);
    
    // Create request structure  
    rkllm_request_t request = {
        .operation = rkllm_proxy_get_operation_by_name("init"),
        .handle_id = 0,
        .params_json = (char*)params_json,
        .params_size = strlen(params_json)
    };    rkllm_result_t exec_result = {0};
    
    printf("Debug: Calling rkllm_proxy_execute...\n");
    int ret = rkllm_proxy_execute(&request, &exec_result);
    
    printf("Debug: rkllm_proxy_execute returned: %d\n", ret);
    printf("Debug: Result: %s\n", exec_result.result_data ? exec_result.result_data : "NULL");
    
    if (ret == 0) {
        printf("✅ QwenVL model loaded successfully!\n");
        printf("📋 Result: %s\n", exec_result.result_data ? exec_result.result_data : "NULL");
        
        // Extract handle_id from result 
        // Simple JSON parsing for handle_id
        const char* handle_start = strstr(exec_result.result_data, "\"handle_id\":");
        if (handle_start) {
            handle_start += 12; // Skip "handle_id":
            uint32_t handle_id = (uint32_t)strtoul(handle_start, NULL, 10);
            printf("🎯 Model handle ID: %u\n", handle_id);
            
            // Test inference with "hello"
            int inference_result = test_qwenvl_inference(handle_id);
            
            // Clean up
            rkllm_proxy_free_result(&exec_result);
            json_object_put(params_obj);
            printf("Debug: Calling rkllm_proxy_shutdown...\n");
            rkllm_proxy_shutdown();
            printf("Debug: Shutdown completed\n");
            return inference_result;
        } else {
            printf("⚠️  No handle_id found in result\n");
            rkllm_proxy_free_result(&exec_result);
            json_object_put(params_obj);
            printf("Debug: Calling rkllm_proxy_shutdown...\n");
            rkllm_proxy_shutdown();
            printf("Debug: Shutdown completed\n");
            return 1;
        }
    } else {
        printf("❌ Failed to load QwenVL model\n");
        printf("📋 Error: %s\n", exec_result.result_data ? exec_result.result_data : "NULL");
        rkllm_proxy_free_result(&exec_result);
        json_object_put(params_obj);
        printf("Debug: Calling rkllm_proxy_shutdown...\n");
        rkllm_proxy_shutdown();
        printf("Debug: Shutdown completed\n");
        return 1;
    }
}

int test_qwenvl_inference(uint32_t handle_id) {
    printf("🚀 Testing QwenVL inference with comprehensive tests...\n");
    
    // Test 1: Simple greeting using json-c
    printf("\n📝 Test 1: Simple greeting\n");
    const char* prompt1 = "Hello! Please introduce yourself briefly.";
    printf("📥 Input: \"%s\"\n", prompt1);
    
    json_object *params1_obj = json_object_new_object();
    json_object *handle_id_obj = json_object_new_int(handle_id);
    json_object *prompt_obj = json_object_new_string(prompt1);
    json_object_object_add(params1_obj, "handle_id", handle_id_obj);
    json_object_object_add(params1_obj, "prompt", prompt_obj);
    
    const char *params1_json = json_object_to_json_string(params1_obj);
    
    rkllm_request_t request1 = {
        .operation = rkllm_proxy_get_operation_by_name("run"),
        .handle_id = handle_id,
        .params_json = (char*)params1_json,
        .params_size = strlen(params1_json)
    };
    
    rkllm_result_t result1 = {0};
    int ret = rkllm_proxy_execute(&request1, &result1);
    
    if (ret == 0) {
        printf("✅ Test 1 successful!\n");
        printf("🤖 Model Response: %s\n", result1.result_data ? result1.result_data : "NULL");
        rkllm_proxy_free_result(&result1);
    } else {
        printf("❌ Test 1 failed\n");
        printf("📋 Error: %s\n", result1.result_data ? result1.result_data : "Unknown error");
        rkllm_proxy_free_result(&result1);
    }
    json_object_put(params1_obj);
    
    // Test 2: Check model status using json-c
    printf("\n📊 Test 2: Model status check\n");
    json_object *status_obj = json_object_new_object();
    json_object *status_handle_id = json_object_new_int(handle_id);
    json_object_object_add(status_obj, "handle_id", status_handle_id);
    
    const char *status_params_json = json_object_to_json_string(status_obj);
    
    rkllm_request_t status_request = {
        .operation = rkllm_proxy_get_operation_by_name("is_running"),
        .handle_id = handle_id,
        .params_json = (char*)status_params_json,
        .params_size = strlen(status_params_json)
    };
    
    rkllm_result_t status_result = {0};
    ret = rkllm_proxy_execute(&status_request, &status_result);
    
    if (ret == 0) {
        printf("✅ Status check successful!\n");
        printf("📊 Model Status: %s\n", status_result.result_data ? status_result.result_data : "NULL");
        rkllm_proxy_free_result(&status_result);
    } else {
        printf("❌ Status check failed\n");
        printf("📋 Error: %s\n", status_result.result_data ? status_result.result_data : "Unknown error");
        rkllm_proxy_free_result(&status_result);
    }
    json_object_put(status_obj);
    
    // Cleanup: Destroy handle to free NPU memory using json-c
    printf("\n🧹 Cleanup: Destroying handle...\n");
    json_object *destroy_obj = json_object_new_object();
    json_object *destroy_handle_id = json_object_new_int(handle_id);
    json_object_object_add(destroy_obj, "handle_id", destroy_handle_id);
    
    const char *destroy_params_json = json_object_to_json_string(destroy_obj);
    
    rkllm_request_t destroy_request = {
        .operation = rkllm_proxy_get_operation_by_name("destroy"),
        .handle_id = handle_id,
        .params_json = (char*)destroy_params_json,
        .params_size = strlen(destroy_params_json)
    };
    
    rkllm_result_t destroy_result = {0};
    ret = rkllm_proxy_execute(&destroy_request, &destroy_result);
    if (ret == 0) {
        printf("✅ Handle destroyed successfully\n");
    } else {
        printf("⚠️  Handle destruction failed\n");
    }
    rkllm_proxy_free_result(&destroy_result);
    json_object_put(destroy_obj);
    
    return 0;
}

int test_integration_qwenvl(void) {
    printf("🧪 QWENVL INTEGRATION TEST\n");
    printf("==========================\n\n");
    
    // Check if model exists
    if (access("/home/x/Projects/nano/models/qwenvl/model.rkllm", F_OK) != 0) {
        printf("❌ QwenVL model not found at /home/x/Projects/nano/models/qwenvl/model.rkllm\n");
        return 1;
    }
    
    int loading_result = test_qwenvl_model_loading();
    
    // Explicit cleanup to free NPU memory
    printf("🧹 Cleaning up QwenVL resources...\n");
    rkllm_proxy_shutdown();
    sleep(5); // Allow NPU memory to be fully released
    
    printf("\n🎉 QwenVL integration test completed!\n");
    return loading_result;
}
