#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Extract string value from JSON by key
 * @param json JSON string to parse
 * @param key Key to search for
 * @param buffer Buffer to store result
 * @param buffer_size Size of buffer
 * @return Pointer to buffer on success, NULL on failure
 */
const char* json_get_string(const char* json, const char* key, char* buffer, size_t buffer_size);

/**
 * @brief Extract integer value from JSON by key
 * @param json JSON string to parse
 * @param key Key to search for
 * @param default_val Default value if key not found
 * @return Integer value or default_val
 */
int json_get_int(const char* json, const char* key, int default_val);

/**
 * @brief Extract double value from JSON by key
 * @param json JSON string to parse
 * @param key Key to search for
 * @param default_val Default value if key not found
 * @return Double value or default_val
 */
double json_get_double(const char* json, const char* key, double default_val);



/**
 * @brief Extract string value from JSON by key (safe, null-terminated)
 * @param json JSON string to parse
 * @param key Key to search for
 * @param buffer Buffer to store result
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on failure
 */
int json_extract_string_safe(const char* json, const char* key, char* buffer, size_t buffer_size);


/**
 * @brief Extract JSON object value by key
 * @param json JSON string to parse
 * @param key Key to search for
 * @param buffer Buffer to store result
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on failure
 */
int json_extract_object(const char* json, const char* key, char* buffer, size_t buffer_size);

/**
 * @brief Extract uint32_t value from JSON by key
 * @param json JSON string to parse
 * @param key Key to search for
 * @param default_val Default value if key not found
 * @return uint32_t value or default_val
 */
uint32_t json_get_uint32(const char* json, const char* key, uint32_t default_val);
