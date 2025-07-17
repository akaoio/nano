#pragma once

#include <stddef.h>

/**
 * @brief Create standardized error result
 * @param code Error code 
 * @param message Error message
 * @return Allocated error result string (caller must free)
 */
char* create_error_result(int code, const char* message);

/**
 * @brief Create standardized success result
 * @param message Success message
 * @return Allocated success result string (caller must free)
 */
char* create_success_result(const char* message);

/**
 * @brief Set error result in result structure
 * @param result Pointer to result structure
 * @param code Error code
 * @param message Error message
 */
#define SET_ERROR_RESULT(result, code, message) do { \
    (result)->result_data = create_error_result(code, message); \
    (result)->result_size = strlen((result)->result_data); \
} while(0)

/**
 * @brief Set success result in result structure
 * @param result Pointer to result structure
 * @param message Success message
 */
#define SET_SUCCESS_RESULT(result, message) do { \
    (result)->result_data = create_success_result(message); \
    (result)->result_size = strlen((result)->result_data); \
} while(0)

/**
 * @brief Validate handle and return error if invalid
 * @param handle_id Handle ID to validate
 * @param result Result structure to populate on error
 * @return 0 if valid, -1 if invalid
 */
#define VALIDATE_HANDLE_OR_ERROR(handle_id, result) do { \
    if (!is_valid_handle_id(handle_id)) { \
        SET_ERROR_RESULT(result, -1, "Invalid handle ID"); \
        return -1; \
    } \
} while(0)

/**
 * @brief Validate parameters and return error if invalid
 * @param condition Condition to check
 * @param result Result structure to populate on error
 * @param message Error message
 */
#define VALIDATE_PARAM_OR_ERROR(condition, result, message) do { \
    if (!(condition)) { \
        SET_ERROR_RESULT(result, -1, message); \
        return -1; \
    } \
} while(0)


