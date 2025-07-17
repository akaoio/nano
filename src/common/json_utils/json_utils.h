#pragma once

#include <stddef.h>
#include <stdbool.h>

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
 * @brief Extract float value from JSON by key
 * @param json JSON string to parse
 * @param key Key to search for
 * @param default_val Default value if key not found
 * @return Float value or default_val
 */
float json_get_float(const char* json, const char* key, float default_val);

/**
 * @brief Extract boolean value from JSON by key
 * @param json JSON string to parse
 * @param key Key to search for
 * @param default_val Default value if key not found
 * @return Boolean value or default_val
 */
bool json_get_bool(const char* json, const char* key, bool default_val);
