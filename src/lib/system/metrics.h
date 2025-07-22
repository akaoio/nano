#ifndef SYSTEM_METRICS_H
#define SYSTEM_METRICS_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>

/**
 * @file metrics.h
 * @brief Performance Metrics Collection System
 * 
 * Provides comprehensive metrics collection compatible with Prometheus format,
 * including counters, gauges, and histograms.
 */

#define METRICS_MAX_NAME_LEN 64
#define METRICS_MAX_HELP_LEN 256
#define METRICS_MAX_LABEL_PAIRS 10
#define METRICS_MAX_LABEL_LEN 64

typedef struct {
    char name[METRICS_MAX_NAME_LEN];
    char help[METRICS_MAX_HELP_LEN];
    double value;
    uint64_t timestamp;
    char labels[METRICS_MAX_LABEL_PAIRS][2][METRICS_MAX_LABEL_LEN]; // [pair][key/value][string]
    int label_count;
} metric_t;

typedef struct {
    metric_t* metrics;
    size_t metric_count;
    size_t max_metrics;
    pthread_mutex_t metrics_mutex;
    bool initialized;
    uint64_t start_time;
    uint64_t total_metric_updates;
} metrics_system_t;

/**
 * @brief Initialize the metrics system
 * @return 0 on success, -1 on failure
 */
int metrics_init(void);

/**
 * @brief Shutdown the metrics system
 */
void metrics_shutdown(void);

/**
 * @brief Check if metrics system is initialized
 * @return true if initialized, false otherwise
 */
bool metrics_is_initialized(void);

/**
 * @brief Increment a counter metric
 * @param name Metric name
 * @param labels Array of label key-value pairs (can be NULL)
 * @param label_count Number of label pairs
 */
void metrics_counter_inc(const char* name, const char* labels[][2], int label_count);

/**
 * @brief Add value to a counter metric
 * @param name Metric name
 * @param value Value to add
 * @param labels Array of label key-value pairs (can be NULL)
 * @param label_count Number of label pairs
 */
void metrics_counter_add(const char* name, double value, const char* labels[][2], int label_count);

/**
 * @brief Set a gauge metric value
 * @param name Metric name
 * @param value New value
 * @param labels Array of label key-value pairs (can be NULL)
 * @param label_count Number of label pairs
 */
void metrics_gauge_set(const char* name, double value, const char* labels[][2], int label_count);

/**
 * @brief Increment a gauge metric
 * @param name Metric name
 * @param labels Array of label key-value pairs (can be NULL)
 * @param label_count Number of label pairs
 */
void metrics_gauge_inc(const char* name, const char* labels[][2], int label_count);

/**
 * @brief Decrement a gauge metric
 * @param name Metric name
 * @param labels Array of label key-value pairs (can be NULL)
 * @param label_count Number of label pairs
 */
void metrics_gauge_dec(const char* name, const char* labels[][2], int label_count);

/**
 * @brief Observe a value for histogram (simplified implementation)
 * @param name Metric name (will create _count and _sum metrics)
 * @param value Value to observe
 * @param labels Array of label key-value pairs (can be NULL)
 * @param label_count Number of label pairs
 */
void metrics_histogram_observe(const char* name, double value, const char* labels[][2], int label_count);

/**
 * @brief Export metrics in Prometheus format
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return Number of bytes written, -1 on error
 */
int metrics_export_prometheus(char* output, size_t output_size);

/**
 * @brief Get metrics system statistics
 * @param metric_count Output: number of metrics
 * @param total_updates Output: total metric updates
 * @param uptime_ms Output: system uptime in milliseconds
 * @return 0 on success, -1 on failure
 */
int metrics_get_system_stats(size_t* metric_count, uint64_t* total_updates, uint64_t* uptime_ms);

/**
 * @brief Clear all metrics
 */
void metrics_clear_all(void);

/**
 * @brief Get specific metric value
 * @param name Metric name
 * @param labels Array of label key-value pairs (can be NULL)
 * @param label_count Number of label pairs
 * @param value Output: metric value
 * @return 0 on success, -1 if not found
 */
int metrics_get_value(const char* name, const char* labels[][2], int label_count, double* value);

// Standard server metrics - convenience functions
void metrics_request_received(const char* transport, const char* method);
void metrics_request_completed(const char* transport, const char* method, double duration_ms);
void metrics_request_failed(const char* transport, const char* method, const char* error_type);
void metrics_connection_opened(const char* transport);
void metrics_connection_closed(const char* transport);
void metrics_memory_usage(size_t bytes);

#endif // SYSTEM_METRICS_H