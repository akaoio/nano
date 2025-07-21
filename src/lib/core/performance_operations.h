#ifndef PERFORMANCE_OPERATIONS_H
#define PERFORMANCE_OPERATIONS_H

#include "performance_monitor.h"

// Performance monitoring operations exposed via JSON-RPC API
// These operations allow real-time monitoring and control of performance metrics

/**
 * @brief Get performance statistics via JSON-RPC
 * @param params_json Input parameters with optional category filter
 * @param result_json Output JSON with performance statistics
 * @return 0 on success, -1 on error
 */
int performance_operation_get_statistics(const char* params_json, char** result_json);

/**
 * @brief Generate comprehensive performance report via JSON-RPC
 * @param params_json Input parameters with report options
 * @param result_json Output JSON with detailed performance report
 * @return 0 on success, -1 on error
 */
int performance_operation_generate_report(const char* params_json, char** result_json);

/**
 * @brief Create performance counter via JSON-RPC
 * @param params_json Input parameters with counter name, category, type
 * @param result_json Output JSON with counter ID
 * @return 0 on success, -1 on error
 */
int performance_operation_create_counter(const char* params_json, char** result_json);

/**
 * @brief Create performance timer via JSON-RPC
 * @param params_json Input parameters with timer name, category
 * @param result_json Output JSON with timer ID
 * @return 0 on success, -1 on error
 */
int performance_operation_create_timer(const char* params_json, char** result_json);

/**
 * @brief Start performance timer via JSON-RPC
 * @param params_json Input parameters with timer ID
 * @param result_json Output JSON with confirmation
 * @return 0 on success, -1 on error
 */
int performance_operation_start_timer(const char* params_json, char** result_json);

/**
 * @brief Stop performance timer via JSON-RPC
 * @param params_json Input parameters with timer ID
 * @param result_json Output JSON with elapsed time
 * @return 0 on success, -1 on error
 */
int performance_operation_stop_timer(const char* params_json, char** result_json);

/**
 * @brief Set performance alert via JSON-RPC
 * @param params_json Input parameters with alert configuration
 * @param result_json Output JSON with alert ID
 * @return 0 on success, -1 on error
 */
int performance_operation_set_alert(const char* params_json, char** result_json);

/**
 * @brief Reset all performance metrics via JSON-RPC
 * @param params_json Input parameters (empty object)
 * @param result_json Output JSON with confirmation
 * @return 0 on success, -1 on error
 */
int performance_operation_reset_metrics(const char* params_json, char** result_json);

/**
 * @brief Get system resource utilization via JSON-RPC
 * @param params_json Input parameters (empty object)
 * @param result_json Output JSON with system resources
 * @return 0 on success, -1 on error
 */
int performance_operation_get_system_resources(const char* params_json, char** result_json);

/**
 * @brief Configure performance monitoring via JSON-RPC
 * @param params_json Input parameters with monitoring configuration
 * @param result_json Output JSON with confirmation
 * @return 0 on success, -1 on error
 */
int performance_operation_configure_monitoring(const char* params_json, char** result_json);

#endif // PERFORMANCE_OPERATIONS_H