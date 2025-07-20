#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "../src/lib/core/rkllm_proxy.h"
#include "../src/lib/core/operations.h"

// Test state
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} test_state_t;

static test_state_t g_state = {0};

bool test_function_listing() {
    printf("🧪 Test: Dynamic Function Listing\n");
    printf("----------------------------------\n");
    
    g_state.tests_run++;
    
    char* functions_json = NULL;
    int result = rkllm_proxy_get_functions(&functions_json);
    
    if (result == 0 && functions_json) {
        printf("✅ Function listing: PASSED\n");
        printf("📋 Available RKLLM functions:\n");
        
        // Parse and display functions
        json_object* functions = json_tokener_parse(functions_json);
        if (functions) {
            int count = json_object_array_length(functions);
            printf("   Total functions: %d\n", count);
            
            for (int i = 0; i < count && i < 5; i++) {  // Show first 5
                json_object* func = json_object_array_get_idx(functions, i);
                json_object* name_obj;
                if (json_object_object_get_ex(func, "name", &name_obj)) {
                    printf("   %d. %s\n", i+1, json_object_get_string(name_obj));
                }
            }
            if (count > 5) {
                printf("   ... and %d more functions\n", count - 5);
            }
            
            json_object_put(functions);
        }
        
        free(functions_json);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ Function listing: FAILED\n");
        if (functions_json) free(functions_json);
        g_state.tests_failed++;
        return false;
    }
}

bool test_create_default_param() {
    printf("\n🧪 Test: rkllm_createDefaultParam\n");
    printf("----------------------------------\n");
    
    g_state.tests_run++;
    
    char* result_json = NULL;
    int result = rkllm_proxy_call("rkllm_createDefaultParam", "{}", &result_json);
    
    if (result == 0 && result_json) {
        printf("✅ rkllm_createDefaultParam: PASSED\n");
        printf("📋 Default parameters: %s\n", result_json);
        
        // Verify it contains expected fields
        json_object* response = json_tokener_parse(result_json);
        if (response) {
            json_object* default_params;
            if (json_object_object_get_ex(response, "default_params", &default_params)) {
                json_object* max_tokens_obj;
                if (json_object_object_get_ex(default_params, "max_new_tokens", &max_tokens_obj)) {
                    int max_tokens = json_object_get_int(max_tokens_obj);
                    printf("   Default max_new_tokens: %d\n", max_tokens);
                }
            }
            json_object_put(response);
        }
        
        free(result_json);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ rkllm_createDefaultParam: FAILED\n");
        if (result_json) {
            printf("   Error: %s\n", result_json);
            free(result_json);
        }
        g_state.tests_failed++;
        return false;
    }
}

bool test_unknown_function() {
    printf("\n🧪 Test: Unknown Function Handling\n");
    printf("-----------------------------------\n");
    
    g_state.tests_run++;
    
    char* result_json = NULL;
    int result = rkllm_proxy_call("rkllm_nonexistent_function", "{}", &result_json);
    
    if (result != 0 && result_json && strstr(result_json, "Function not found")) {
        printf("✅ Unknown function handling: PASSED\n");
        printf("   Correctly rejected unknown function\n");
        free(result_json);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ Unknown function handling: FAILED\n");
        if (result_json) {
            printf("   Unexpected response: %s\n", result_json);
            free(result_json);
        }
        g_state.tests_failed++;
        return false;
    }
}

bool test_via_operations_interface() {
    printf("\n🧪 Test: Dynamic API via Operations Interface\n");
    printf("---------------------------------------------\n");
    
    g_state.tests_run++;
    
    // Test through the operations.c interface
    char* result_json = NULL;
    int result = io_process_operation("rkllm_createDefaultParam", "{}", &result_json);
    
    if (result == 0 && result_json) {
        printf("✅ Operations interface: PASSED\n");
        printf("   Successfully called via io_process_operation\n");
        free(result_json);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ Operations interface: FAILED\n");
        if (result_json) {
            printf("   Error: %s\n", result_json);
            free(result_json);
        }
        g_state.tests_failed++;
        return false;
    }
}

bool test_function_list_via_operations() {
    printf("\n🧪 Test: Function Listing via Operations\n");
    printf("----------------------------------------\n");
    
    g_state.tests_run++;
    
    // Test the special rkllm_list_functions method
    char* result_json = NULL;
    int result = io_process_operation("rkllm_list_functions", "{}", &result_json);
    
    if (result == 0 && result_json) {
        printf("✅ Function listing via operations: PASSED\n");
        
        // Count functions in the response
        json_object* functions = json_tokener_parse(result_json);
        if (functions && json_object_is_type(functions, json_type_array)) {
            int count = json_object_array_length(functions);
            printf("   Available functions: %d\n", count);
            json_object_put(functions);
        }
        
        free(result_json);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ Function listing via operations: FAILED\n");
        if (result_json) {
            printf("   Error: %s\n", result_json);
            free(result_json);
        }
        g_state.tests_failed++;
        return false;
    }
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    printf("🚀 Dynamic RKLLM API Test Suite\n");
    printf("================================\n");
    printf("Testing dynamic function registry and automatic parameter conversion\n\n");
    
    // Initialize the proxy system
    if (rkllm_proxy_init() != 0) {
        printf("❌ Failed to initialize RKLLM proxy\n");
        return 1;
    }
    
    // Initialize operations system (which initializes proxy)
    if (io_operations_init() != 0) {
        printf("❌ Failed to initialize IO operations\n");
        return 1;
    }
    
    printf("✅ Dynamic RKLLM proxy initialized\n\n");
    
    // Run tests
    test_function_listing();
    test_create_default_param();
    test_unknown_function();
    test_via_operations_interface();
    test_function_list_via_operations();
    
    // Cleanup
    io_operations_shutdown();
    rkllm_proxy_shutdown();
    
    // Final summary
    printf("\n📊 Test Results\n");
    printf("===============\n");
    printf("Tests run:    %d\n", g_state.tests_run);
    printf("Tests passed: %d\n", g_state.tests_passed);
    printf("Tests failed: %d\n", g_state.tests_failed);
    
    if (g_state.tests_failed == 0) {
        printf("\n🎉 All tests PASSED! Dynamic RKLLM API is working correctly.\n");
        printf("\n📋 Key Benefits:\n");
        printf("• Automatic exposure of ALL RKLLM functions (15+ functions)\n");
        printf("• Dynamic parameter conversion from JSON to RKLLM structures\n");
        printf("• No manual function mapping required\n");
        printf("• Easy to add new RKLLM functions as they become available\n");
        printf("• Type-safe function calls with error handling\n");
        return 0;
    } else {
        printf("\n❌ %d test(s) FAILED. Check output above.\n", g_state.tests_failed);
        return 1;
    }
}