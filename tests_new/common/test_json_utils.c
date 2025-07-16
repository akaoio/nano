#include "json_utils/json_utils.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void test_json_get_string() {
    const char* json = "{\"name\":\"test\",\"value\":\"hello\"}";
    char buffer[64];
    
    // Test valid key
    const char* result = json_get_string(json, "name", buffer, sizeof(buffer));
    assert(result != NULL);
    assert(strcmp(result, "test") == 0);
    
    // Test invalid key
    result = json_get_string(json, "invalid", buffer, sizeof(buffer));
    assert(result == NULL);
    
    // Test NULL inputs
    result = json_get_string(NULL, "name", buffer, sizeof(buffer));
    assert(result == NULL);
    
    printf("✓ test_json_get_string passed\n");
}

void test_json_get_int() {
    const char* json = "{\"count\":42,\"negative\":-10}";
    
    // Test valid integer
    int result = json_get_int(json, "count", 0);
    assert(result == 42);
    
    // Test negative integer
    result = json_get_int(json, "negative", 0);
    assert(result == -10);
    
    // Test missing key (should return default)
    result = json_get_int(json, "missing", 99);
    assert(result == 99);
    
    printf("✓ test_json_get_int passed\n");
}

void test_json_get_double() {
    const char* json = "{\"pi\":3.14159,\"zero\":0.0}";
    
    // Test valid double
    double result = json_get_double(json, "pi", 0.0);
    assert(result > 3.14 && result < 3.15);
    
    // Test zero double
    result = json_get_double(json, "zero", 1.0);
    assert(result == 0.0);
    
    // Test missing key (should return default)
    result = json_get_double(json, "missing", 2.5);
    assert(result == 2.5);
    
    printf("✓ test_json_get_double passed\n");
}

int main() {
    printf("Running json_utils tests...\n");
    
    test_json_get_string();
    test_json_get_int();
    test_json_get_double();
    
    printf("All json_utils tests passed!\n");
    return 0;
}
