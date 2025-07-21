#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

// Maximum number of performance counters and metrics
#define PERF_MAX_COUNTERS 64
#define PERF_MAX_TIMERS 32
#define PERF_MAX_HISTOGRAMS 16
#define PERF_HISTORY_SIZE 1000
#define PERF_MAX_THREADS 32

// Performance metric types
typedef enum {
    PERF_METRIC_COUNTER = 0,     // Monotonic counter (e.g., total requests)
    PERF_METRIC_GAUGE,           // Current value (e.g., active connections)
    PERF_METRIC_HISTOGRAM,       // Distribution of values (e.g., response times)
    PERF_METRIC_RATE,            // Rate per second (e.g., requests/sec)
    PERF_METRIC_TIMER,           // Timing measurements
} performance_metric_type_t;

// Performance categories for organization
typedef enum {
    PERF_CATEGORY_RKLLM = 0,     // RKLLM function calls and inference
    PERF_CATEGORY_STREAMING,     // Streaming operations
    PERF_CATEGORY_TRANSPORT,     // Transport layer operations
    PERF_CATEGORY_MEMORY,        // Memory operations
    PERF_CATEGORY_IO,            // I/O operations
    PERF_CATEGORY_SYSTEM,        // System-level metrics
    PERF_CATEGORY_CUSTOM,        // Custom application metrics
    PERF_CATEGORY_COUNT
} performance_category_t;

// Performance counter structure
typedef struct {
    char name[64];
    performance_category_t category;
    performance_metric_type_t type;
    uint64_t value;                    // Current value
    uint64_t total;                    // Total accumulated value
    uint64_t min_value;                // Minimum value seen
    uint64_t max_value;                // Maximum value seen
    double average;                    // Running average
    uint64_t sample_count;             // Number of samples
    uint64_t last_update_time;         // Last update timestamp
    pthread_mutex_t mutex;             // Thread safety
    bool active;
} performance_counter_t;

// Performance timer for measuring operation durations
typedef struct {
    char name[64];
    performance_category_t category;
    uint64_t start_time;
    uint64_t total_time;               // Total accumulated time
    uint64_t min_time;                 // Minimum duration
    uint64_t max_time;                 // Maximum duration  
    double avg_time;                   // Average duration
    uint64_t call_count;               // Number of calls
    uint64_t active_timers;            // Currently active timers
    pthread_mutex_t mutex;
    bool active;
} performance_timer_t;

// Histogram bucket for distribution analysis
typedef struct {
    uint64_t lower_bound;              // Lower bound of bucket
    uint64_t upper_bound;              // Upper bound of bucket
    uint64_t count;                    // Number of samples in bucket
} histogram_bucket_t;

// Performance histogram for value distribution
typedef struct {
    char name[64];
    performance_category_t category;
    histogram_bucket_t buckets[20];    // 20 buckets for distribution
    int bucket_count;
    uint64_t total_samples;
    uint64_t sum;                      // Sum of all values
    double mean;                       // Mean value
    double std_dev;                    // Standard deviation
    uint64_t percentile_95;            // 95th percentile
    uint64_t percentile_99;            // 99th percentile
    pthread_mutex_t mutex;
    bool active;
} performance_histogram_t;

// Thread-specific performance context
typedef struct {
    uint32_t thread_id;
    char thread_name[32];
    uint64_t cpu_time_ns;              // CPU time used by thread
    uint64_t operations_count;         // Operations performed
    uint64_t bytes_processed;          // Bytes processed
    uint64_t start_time;               // Thread start time
    bool active;
} thread_performance_t;

// System resource utilization
typedef struct {
    double cpu_usage_percent;          // CPU utilization percentage
    uint64_t memory_rss_bytes;         // Resident set size
    uint64_t memory_vms_bytes;         // Virtual memory size
    uint32_t open_file_descriptors;    // Number of open FDs
    uint32_t thread_count;             // Number of threads
    double load_average_1m;            // 1-minute load average
    uint64_t network_bytes_in;         // Network bytes received
    uint64_t network_bytes_out;        // Network bytes sent
    uint64_t disk_bytes_read;          // Disk bytes read
    uint64_t disk_bytes_written;       // Disk bytes written
} system_resources_t;

// Performance alert thresholds
typedef struct {
    char name[64];
    performance_category_t category;
    double threshold_value;            // Alert threshold
    bool threshold_exceeded;           // Is threshold currently exceeded
    uint64_t alert_count;              // Number of times alert triggered
    uint64_t last_alert_time;          // Last alert timestamp
    void (*alert_callback)(const char* name, double current_value, double threshold);
} performance_alert_t;

// Main performance monitoring system
typedef struct {
    bool initialized;
    bool monitoring_enabled;
    bool profiling_enabled;
    
    // Performance metrics
    performance_counter_t counters[PERF_MAX_COUNTERS];
    performance_timer_t timers[PERF_MAX_TIMERS];
    performance_histogram_t histograms[PERF_MAX_HISTOGRAMS];
    
    // Thread tracking
    thread_performance_t threads[PERF_MAX_THREADS];
    int active_counter_count;
    int active_timer_count;
    int active_histogram_count;
    int active_thread_count;
    
    // System monitoring
    system_resources_t current_resources;
    system_resources_t peak_resources;
    
    // Performance alerts
    performance_alert_t alerts[PERF_MAX_COUNTERS];
    int active_alert_count;
    
    // Global statistics
    uint64_t system_start_time;
    uint64_t total_operations;
    uint64_t total_bytes_processed;
    uint64_t total_errors;
    double overall_throughput;         // Operations per second
    
    // Sampling and collection
    uint32_t sample_interval_ms;       // Sampling interval
    pthread_t monitoring_thread;       // Background monitoring thread
    bool monitoring_thread_active;
    
    // Thread safety
    pthread_mutex_t monitor_mutex;
    
} performance_monitor_t;

// Function declarations

/**
 * @brief Initialize performance monitoring system
 * @param enable_profiling Enable detailed profiling
 * @param sample_interval_ms Sampling interval in milliseconds
 * @return 0 on success, -1 on error
 */
int performance_monitor_init(bool enable_profiling, uint32_t sample_interval_ms);

/**
 * @brief Shutdown performance monitoring system
 * @return 0 on success, -1 on error
 */
int performance_monitor_shutdown(void);

/**
 * @brief Create performance counter
 * @param name Counter name
 * @param category Performance category
 * @param type Metric type
 * @return Counter ID or -1 on error
 */
int performance_counter_create(const char* name, performance_category_t category, 
                              performance_metric_type_t type);

/**
 * @brief Increment performance counter
 * @param counter_id Counter ID
 * @param value Value to add
 * @return 0 on success, -1 on error
 */
int performance_counter_increment(int counter_id, uint64_t value);

/**
 * @brief Set performance counter value
 * @param counter_id Counter ID
 * @param value New value
 * @return 0 on success, -1 on error
 */
int performance_counter_set(int counter_id, uint64_t value);

/**
 * @brief Create performance timer
 * @param name Timer name
 * @param category Performance category
 * @return Timer ID or -1 on error
 */
int performance_timer_create(const char* name, performance_category_t category);

/**
 * @brief Start performance timer
 * @param timer_id Timer ID
 * @return 0 on success, -1 on error
 */
int performance_timer_start(int timer_id);

/**
 * @brief Stop performance timer
 * @param timer_id Timer ID
 * @return Elapsed time in nanoseconds, or 0 on error
 */
uint64_t performance_timer_stop(int timer_id);

/**
 * @brief Create performance histogram
 * @param name Histogram name
 * @param category Performance category
 * @param min_value Minimum expected value
 * @param max_value Maximum expected value
 * @return Histogram ID or -1 on error
 */
int performance_histogram_create(const char* name, performance_category_t category,
                                uint64_t min_value, uint64_t max_value);

/**
 * @brief Record value in histogram
 * @param histogram_id Histogram ID
 * @param value Value to record
 * @return 0 on success, -1 on error
 */
int performance_histogram_record(int histogram_id, uint64_t value);

/**
 * @brief Register thread for performance tracking
 * @param thread_name Thread name
 * @return Thread performance ID or -1 on error
 */
int performance_thread_register(const char* thread_name);

/**
 * @brief Update thread performance metrics
 * @param thread_perf_id Thread performance ID
 * @param operations_done Operations completed
 * @param bytes_processed Bytes processed
 * @return 0 on success, -1 on error
 */
int performance_thread_update(int thread_perf_id, uint64_t operations_done, 
                             uint64_t bytes_processed);

/**
 * @brief Get current system resource utilization
 * @param resources Output structure for resources
 * @return 0 on success, -1 on error
 */
int performance_get_system_resources(system_resources_t* resources);

/**
 * @brief Set performance alert threshold
 * @param name Alert name
 * @param category Performance category
 * @param threshold Alert threshold
 * @param callback Alert callback function
 * @return Alert ID or -1 on error
 */
int performance_alert_set(const char* name, performance_category_t category,
                         double threshold, void (*callback)(const char*, double, double));

/**
 * @brief Get performance statistics as JSON
 * @param category Filter by category (PERF_CATEGORY_COUNT for all)
 * @param include_histograms Include histogram data
 * @param stats_json Output JSON string
 * @return 0 on success, -1 on error
 */
int performance_get_statistics(performance_category_t category, bool include_histograms,
                              char** stats_json);

/**
 * @brief Generate performance report as JSON
 * @param include_thread_details Include per-thread details
 * @param include_system_info Include system resource info
 * @param report_json Output JSON report
 * @return 0 on success, -1 on error
 */
int performance_generate_report(bool include_thread_details, bool include_system_info,
                               char** report_json);

/**
 * @brief Reset all performance metrics
 * @return 0 on success, -1 on error
 */
int performance_reset_metrics(void);

// Convenience macros for common operations
#define PERF_COUNTER_INC(id, val) performance_counter_increment(id, val)
#define PERF_COUNTER_SET(id, val) performance_counter_set(id, val)

#define PERF_TIMER_START(id) performance_timer_start(id)
#define PERF_TIMER_STOP(id) performance_timer_stop(id)

#define PERF_HISTOGRAM_RECORD(id, val) performance_histogram_record(id, val)

// RAII-style timer macro for automatic timing
#define PERF_TIME_FUNCTION(timer_id) \
    __attribute__((cleanup(performance_timer_stop_cleanup))) int timer_cleanup_##timer_id = timer_id; \
    performance_timer_start(timer_id);

// Category name helpers
const char* performance_category_to_string(performance_category_t category);
performance_category_t performance_category_from_string(const char* category_str);

#endif // PERFORMANCE_MONITOR_H