#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "json_utils.h"

void test_json_get_string() {
    printf("Testing json_get_string...\n");
    
    const char* json = "{\"name\":\"test\",\"value\":\"hello world\"}";
    char buffer[256];
    
    // Test valid string extraction
    const char* result = json_get_string(json, "name", buffer, sizeof(buffer));
    assert(result != NULL);
    assert(strcmp(result, "test") == 0);
    
    // Test another valid string extraction
    result = json_get_string(json, "value", buffer, sizeof(buffer));
    assert(result != NULL);
    assert(strcmp(result, "hello world") == 0);
    
    // Test non-existent key
    result = json_get_string(json, "nonexistent", buffer, sizeof(buffer));
    assert(result == NULL);
    
    printf("json_get_string tests passed!\n");
}

void test_json_get_int() {
    printf("Testing json_get_int...\n");
    
    const char* json = "{\"number\":42,\"negative\":-10}";
    
    // Test valid integer extraction
    int result = json_get_int(json, "number", 0);
    assert(result == 42);
    
    // Test negative integer
    result = json_get_int(json, "negative", 0);
    assert(result == -10);
    
    // Test non-existent key (should return default)
    result = json_get_int(json, "nonexistent", 999);
    assert(result == 999);
    
    printf("json_get_int tests passed!\n");
}

void test_json_get_double() {
    printf("Testing json_get_double...\n");
    
    const char* json = "{\"pi\":3.14159,\"temp\":-2.5}";
    
    // Test valid double extraction
    double result = json_get_double(json, "pi", 0.0);
    assert(result > 3.14 && result < 3.15);
    
    // Test negative double
    result = json_get_double(json, "temp", 0.0);
    assert(result == -2.5);
    
    // Test non-existent key (should return default)
    result = json_get_double(json, "nonexistent", 99.9);
    assert(result == 99.9);
    
    printf("json_get_double tests passed!\n");
}

int main() {
    printf("Running json_utils tests...\n\n");
    
    test_json_get_string();
    test_json_get_int();
    test_json_get_double();
    
    printf("\nAll json_utils tests passed!\n");
    return 0;
}
