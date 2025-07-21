#ifndef MEMORY_MANAGEMENT_OPERATIONS_H
#define MEMORY_MANAGEMENT_OPERATIONS_H

#include "memory_manager.h"

// Memory management operations exposed via JSON-RPC API
// These operations allow monitoring and controlling memory usage

/**
 * @brief Get memory statistics via JSON-RPC
 * @param params_json Input parameters (empty object)
 * @param result_json Output JSON with memory statistics
 * @return 0 on success, -1 on error
 */
int memory_operation_get_statistics(const char* params_json, char** result_json);

/**
 * @brief Check for memory leaks via JSON-RPC
 * @param params_json Input parameters (empty object)
 * @param result_json Output JSON with leak report
 * @return 0 on success, -1 on error
 */
int memory_operation_check_leaks(const char* params_json, char** result_json);

/**
 * @brief Get pool statistics via JSON-RPC
 * @param params_json Input parameters with pool name
 * @param result_json Output JSON with pool statistics
 * @return 0 on success, -1 on error
 */
int memory_operation_get_pool_stats(const char* params_json, char** result_json);

/**
 * @brief Trigger garbage collection via JSON-RPC
 * @param params_json Input parameters (empty object)
 * @param result_json Output JSON with GC results
 * @return 0 on success, -1 on error
 */
int memory_operation_garbage_collect(const char* params_json, char** result_json);

/**
 * @brief Set memory pressure threshold via JSON-RPC
 * @param params_json Input parameters with threshold
 * @param result_json Output JSON with confirmation
 * @return 0 on success, -1 on error
 */
int memory_operation_set_pressure_threshold(const char* params_json, char** result_json);

/**
 * @brief Validate memory integrity via JSON-RPC
 * @param params_json Input parameters with validation options
 * @param result_json Output JSON with validation results
 * @return 0 on success, -1 on error
 */
int memory_operation_validate_integrity(const char* params_json, char** result_json);

/**
 * @brief Get memory allocation breakdown by type via JSON-RPC
 * @param params_json Input parameters (empty object)
 * @param result_json Output JSON with allocation breakdown
 * @return 0 on success, -1 on error
 */
int memory_operation_get_allocation_breakdown(const char* params_json, char** result_json);

#endif // MEMORY_MANAGEMENT_OPERATIONS_H