#define _POSIX_C_SOURCE 199309L
#include "performance_monitor.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <sys/resource.h>

// Global performance monitor instance
static performance_monitor_t g_performance_monitor = {0};

// Helper functions
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

static uint32_t get_thread_id(void) {
    return (uint32_t)pthread_self();
}

// Category name conversion
const char* performance_category_to_string(performance_category_t category) {
    switch (category) {
        case PERF_CATEGORY_RKLLM: return "rkllm";
        case PERF_CATEGORY_STREAMING: return "streaming";
        case PERF_CATEGORY_TRANSPORT: return "transport";
        case PERF_CATEGORY_MEMORY: return "memory";
        case PERF_CATEGORY_IO: return "io";
        case PERF_CATEGORY_SYSTEM: return "system";
        case PERF_CATEGORY_CUSTOM: return "custom";
        default: return "unknown";
    }
}

performance_category_t performance_category_from_string(const char* category_str) {
    if (!category_str) return PERF_CATEGORY_SYSTEM;
    
    if (strcmp(category_str, "rkllm") == 0) return PERF_CATEGORY_RKLLM;
    if (strcmp(category_str, "streaming") == 0) return PERF_CATEGORY_STREAMING;
    if (strcmp(category_str, "transport") == 0) return PERF_CATEGORY_TRANSPORT;
    if (strcmp(category_str, "memory") == 0) return PERF_CATEGORY_MEMORY;
    if (strcmp(category_str, "io") == 0) return PERF_CATEGORY_IO;
    if (strcmp(category_str, "system") == 0) return PERF_CATEGORY_SYSTEM;
    if (strcmp(category_str, "custom") == 0) return PERF_CATEGORY_CUSTOM;
    
    return PERF_CATEGORY_SYSTEM;
}

// System resource monitoring
static int collect_system_resources(system_resources_t* resources) {
    if (!resources) return -1;
    
    memset(resources, 0, sizeof(system_resources_t));
    
    // Get resource usage
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        resources->memory_rss_bytes = usage.ru_maxrss * 1024; // Convert KB to bytes
        resources->cpu_usage_percent = 0.0; // Would need more complex calculation
    }
    
    // Get load average (simplified for portability)
    resources->load_average_1m = 0.0; // Would need platform-specific implementation
    
    // Count threads (simplified - would read from /proc/self/status in real implementation)
    resources->thread_count = g_performance_monitor.active_thread_count;
    
    // File descriptors (would read from /proc/self/fd in real implementation)
    resources->open_file_descriptors = 10; // Placeholder
    
    return 0;
}

// Background monitoring thread
static void* performance_monitoring_thread(void* arg) {
    (void)arg; // Unused parameter
    
    printf("🔍 Performance monitoring thread started\n");
    
    while (g_performance_monitor.monitoring_thread_active) {
        // Collect system resources
        system_resources_t current_resources;
        if (collect_system_resources(&current_resources) == 0) {
            pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
            g_performance_monitor.current_resources = current_resources;
            
            // Update peak resources
            if (current_resources.memory_rss_bytes > g_performance_monitor.peak_resources.memory_rss_bytes) {
                g_performance_monitor.peak_resources.memory_rss_bytes = current_resources.memory_rss_bytes;
            }
            if (current_resources.cpu_usage_percent > g_performance_monitor.peak_resources.cpu_usage_percent) {
                g_performance_monitor.peak_resources.cpu_usage_percent = current_resources.cpu_usage_percent;
            }
            
            pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        }
        
        // Check performance alerts
        pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
        for (int i = 0; i < g_performance_monitor.active_alert_count; i++) {
            performance_alert_t* alert = &g_performance_monitor.alerts[i];
            
            // Check counters for alert conditions
            for (int c = 0; c < g_performance_monitor.active_counter_count; c++) {
                performance_counter_t* counter = &g_performance_monitor.counters[c];
                if (counter->active && counter->category == alert->category) {
                    double current_value = (double)counter->value;
                    if (current_value > alert->threshold_value) {
                        if (!alert->threshold_exceeded) {
                            alert->threshold_exceeded = true;
                            alert->alert_count++;
                            alert->last_alert_time = get_timestamp_ns();
                            
                            if (alert->alert_callback) {
                                alert->alert_callback(alert->name, current_value, alert->threshold_value);
                            }
                        }
                    } else {
                        alert->threshold_exceeded = false;
                    }
                }
            }
        }
        pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        
        // Sleep for sampling interval
        struct timespec ts = {
            .tv_sec = g_performance_monitor.sample_interval_ms / 1000,
            .tv_nsec = (g_performance_monitor.sample_interval_ms % 1000) * 1000000
        };
        nanosleep(&ts, NULL);
    }
    
    printf("🔍 Performance monitoring thread stopped\n");
    return NULL;
}

// Performance monitor initialization
int performance_monitor_init(bool enable_profiling, uint32_t sample_interval_ms) {
    if (g_performance_monitor.initialized) {
        return 0; // Already initialized
    }
    
    memset(&g_performance_monitor, 0, sizeof(performance_monitor_t));
    
    g_performance_monitor.monitoring_enabled = true;
    g_performance_monitor.profiling_enabled = enable_profiling;
    g_performance_monitor.sample_interval_ms = sample_interval_ms > 0 ? sample_interval_ms : 1000;
    g_performance_monitor.system_start_time = get_timestamp_ns();
    
    // Initialize mutex
    if (pthread_mutex_init(&g_performance_monitor.monitor_mutex, NULL) != 0) {
        return -1;
    }
    
    // Start monitoring thread
    g_performance_monitor.monitoring_thread_active = true;
    if (pthread_create(&g_performance_monitor.monitoring_thread, NULL, 
                      performance_monitoring_thread, NULL) != 0) {
        pthread_mutex_destroy(&g_performance_monitor.monitor_mutex);
        return -1;
    }
    
    g_performance_monitor.initialized = true;
    
    printf("✅ Performance monitoring system initialized (profiling=%s, sample_interval=%u ms)\n",
           enable_profiling ? "enabled" : "disabled", g_performance_monitor.sample_interval_ms);
    
    return 0;
}

int performance_monitor_shutdown(void) {
    if (!g_performance_monitor.initialized) {
        return 0;
    }
    
    // Stop monitoring thread
    g_performance_monitor.monitoring_thread_active = false;
    pthread_join(g_performance_monitor.monitoring_thread, NULL);
    
    // Cleanup counters, timers, histograms
    for (int i = 0; i < g_performance_monitor.active_counter_count; i++) {
        if (g_performance_monitor.counters[i].active) {
            pthread_mutex_destroy(&g_performance_monitor.counters[i].mutex);
        }
    }
    
    for (int i = 0; i < g_performance_monitor.active_timer_count; i++) {
        if (g_performance_monitor.timers[i].active) {
            pthread_mutex_destroy(&g_performance_monitor.timers[i].mutex);
        }
    }
    
    for (int i = 0; i < g_performance_monitor.active_histogram_count; i++) {
        if (g_performance_monitor.histograms[i].active) {
            pthread_mutex_destroy(&g_performance_monitor.histograms[i].mutex);
        }
    }
    
    pthread_mutex_destroy(&g_performance_monitor.monitor_mutex);
    
    g_performance_monitor.initialized = false;
    printf("🧹 Performance monitoring system shutdown\n");
    
    return 0;
}

// Performance counter operations
int performance_counter_create(const char* name, performance_category_t category, 
                              performance_metric_type_t type) {
    if (!g_performance_monitor.initialized || !name) return -1;
    
    pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
    
    if (g_performance_monitor.active_counter_count >= PERF_MAX_COUNTERS) {
        pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        return -1;
    }
    
    int counter_id = g_performance_monitor.active_counter_count++;
    performance_counter_t* counter = &g_performance_monitor.counters[counter_id];
    
    strncpy(counter->name, name, sizeof(counter->name) - 1);
    counter->category = category;
    counter->type = type;
    counter->value = 0;
    counter->total = 0;
    counter->min_value = UINT64_MAX;
    counter->max_value = 0;
    counter->average = 0.0;
    counter->sample_count = 0;
    counter->last_update_time = get_timestamp_ns();
    
    if (pthread_mutex_init(&counter->mutex, NULL) != 0) {
        g_performance_monitor.active_counter_count--;
        pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        return -1;
    }
    
    counter->active = true;
    
    pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
    
    printf("📊 Created performance counter '%s' (category: %s, type: %d)\n", 
           name, performance_category_to_string(category), type);
    
    return counter_id;
}

int performance_counter_increment(int counter_id, uint64_t value) {
    if (!g_performance_monitor.initialized || counter_id < 0 || 
        counter_id >= g_performance_monitor.active_counter_count) return -1;
    
    performance_counter_t* counter = &g_performance_monitor.counters[counter_id];
    if (!counter->active) return -1;
    
    pthread_mutex_lock(&counter->mutex);
    
    counter->value += value;
    counter->total += value;
    counter->sample_count++;
    
    if (value < counter->min_value) {
        counter->min_value = value;
    }
    if (value > counter->max_value) {
        counter->max_value = value;
    }
    
    // Update running average
    counter->average = (double)counter->total / counter->sample_count;
    counter->last_update_time = get_timestamp_ns();
    
    pthread_mutex_unlock(&counter->mutex);
    
    return 0;
}

int performance_counter_set(int counter_id, uint64_t value) {
    if (!g_performance_monitor.initialized || counter_id < 0 || 
        counter_id >= g_performance_monitor.active_counter_count) return -1;
    
    performance_counter_t* counter = &g_performance_monitor.counters[counter_id];
    if (!counter->active) return -1;
    
    pthread_mutex_lock(&counter->mutex);
    
    counter->value = value;
    counter->sample_count++;
    
    if (value < counter->min_value) {
        counter->min_value = value;
    }
    if (value > counter->max_value) {
        counter->max_value = value;
    }
    
    counter->last_update_time = get_timestamp_ns();
    
    pthread_mutex_unlock(&counter->mutex);
    
    return 0;
}

// Performance timer operations
int performance_timer_create(const char* name, performance_category_t category) {
    if (!g_performance_monitor.initialized || !name) return -1;
    
    pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
    
    if (g_performance_monitor.active_timer_count >= PERF_MAX_TIMERS) {
        pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        return -1;
    }
    
    int timer_id = g_performance_monitor.active_timer_count++;
    performance_timer_t* timer = &g_performance_monitor.timers[timer_id];
    
    strncpy(timer->name, name, sizeof(timer->name) - 1);
    timer->category = category;
    timer->start_time = 0;
    timer->total_time = 0;
    timer->min_time = UINT64_MAX;
    timer->max_time = 0;
    timer->avg_time = 0.0;
    timer->call_count = 0;
    timer->active_timers = 0;
    
    if (pthread_mutex_init(&timer->mutex, NULL) != 0) {
        g_performance_monitor.active_timer_count--;
        pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        return -1;
    }
    
    timer->active = true;
    
    pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
    
    printf("⏱️  Created performance timer '%s' (category: %s)\n", 
           name, performance_category_to_string(category));
    
    return timer_id;
}

int performance_timer_start(int timer_id) {
    if (!g_performance_monitor.initialized || timer_id < 0 || 
        timer_id >= g_performance_monitor.active_timer_count) return -1;
    
    performance_timer_t* timer = &g_performance_monitor.timers[timer_id];
    if (!timer->active) return -1;
    
    pthread_mutex_lock(&timer->mutex);
    timer->start_time = get_timestamp_ns();
    timer->active_timers++;
    pthread_mutex_unlock(&timer->mutex);
    
    return 0;
}

uint64_t performance_timer_stop(int timer_id) {
    if (!g_performance_monitor.initialized || timer_id < 0 || 
        timer_id >= g_performance_monitor.active_timer_count) return 0;
    
    performance_timer_t* timer = &g_performance_monitor.timers[timer_id];
    if (!timer->active) return 0;
    
    uint64_t end_time = get_timestamp_ns();
    
    pthread_mutex_lock(&timer->mutex);
    
    if (timer->active_timers > 0 && timer->start_time > 0) {
        uint64_t elapsed = end_time - timer->start_time;
        
        timer->total_time += elapsed;
        timer->call_count++;
        timer->active_timers--;
        
        if (elapsed < timer->min_time) {
            timer->min_time = elapsed;
        }
        if (elapsed > timer->max_time) {
            timer->max_time = elapsed;
        }
        
        timer->avg_time = (double)timer->total_time / timer->call_count;
        timer->start_time = 0;
        
        pthread_mutex_unlock(&timer->mutex);
        return elapsed;
    }
    
    pthread_mutex_unlock(&timer->mutex);
    return 0;
}

// Performance histogram operations
int performance_histogram_create(const char* name, performance_category_t category,
                                uint64_t min_value, uint64_t max_value) {
    if (!g_performance_monitor.initialized || !name || min_value >= max_value) return -1;
    
    pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
    
    if (g_performance_monitor.active_histogram_count >= PERF_MAX_HISTOGRAMS) {
        pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        return -1;
    }
    
    int histogram_id = g_performance_monitor.active_histogram_count++;
    performance_histogram_t* histogram = &g_performance_monitor.histograms[histogram_id];
    
    strncpy(histogram->name, name, sizeof(histogram->name) - 1);
    histogram->category = category;
    histogram->total_samples = 0;
    histogram->sum = 0;
    histogram->mean = 0.0;
    histogram->std_dev = 0.0;
    histogram->percentile_95 = 0;
    histogram->percentile_99 = 0;
    
    // Create 20 buckets with exponential distribution
    uint64_t range = max_value - min_value;
    histogram->bucket_count = 20;
    
    for (int i = 0; i < 20; i++) {
        histogram->buckets[i].lower_bound = min_value + (range * i) / 20;
        histogram->buckets[i].upper_bound = min_value + (range * (i + 1)) / 20;
        histogram->buckets[i].count = 0;
    }
    
    if (pthread_mutex_init(&histogram->mutex, NULL) != 0) {
        g_performance_monitor.active_histogram_count--;
        pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        return -1;
    }
    
    histogram->active = true;
    
    pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
    
    printf("📊 Created performance histogram '%s' (category: %s, range: %lu-%lu)\n", 
           name, performance_category_to_string(category), min_value, max_value);
    
    return histogram_id;
}

int performance_histogram_record(int histogram_id, uint64_t value) {
    if (!g_performance_monitor.initialized || histogram_id < 0 || 
        histogram_id >= g_performance_monitor.active_histogram_count) return -1;
    
    performance_histogram_t* histogram = &g_performance_monitor.histograms[histogram_id];
    if (!histogram->active) return -1;
    
    pthread_mutex_lock(&histogram->mutex);
    
    // Find appropriate bucket
    for (int i = 0; i < histogram->bucket_count; i++) {
        if (value >= histogram->buckets[i].lower_bound && value < histogram->buckets[i].upper_bound) {
            histogram->buckets[i].count++;
            break;
        }
    }
    
    // Update statistics
    histogram->total_samples++;
    histogram->sum += value;
    histogram->mean = (double)histogram->sum / histogram->total_samples;
    
    // Simple percentile calculation (would need more sophisticated algorithm for accuracy)
    if (histogram->total_samples > 0) {
        histogram->percentile_95 = value; // Simplified
        histogram->percentile_99 = value; // Simplified
    }
    
    pthread_mutex_unlock(&histogram->mutex);
    
    return 0;
}

// Thread performance tracking
int performance_thread_register(const char* thread_name) {
    if (!g_performance_monitor.initialized || !thread_name) return -1;
    
    pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
    
    if (g_performance_monitor.active_thread_count >= PERF_MAX_THREADS) {
        pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
        return -1;
    }
    
    int thread_id = g_performance_monitor.active_thread_count++;
    thread_performance_t* thread_perf = &g_performance_monitor.threads[thread_id];
    
    thread_perf->thread_id = get_thread_id();
    strncpy(thread_perf->thread_name, thread_name, sizeof(thread_perf->thread_name) - 1);
    thread_perf->cpu_time_ns = 0;
    thread_perf->operations_count = 0;
    thread_perf->bytes_processed = 0;
    thread_perf->start_time = get_timestamp_ns();
    thread_perf->active = true;
    
    pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
    
    printf("🧵 Registered thread '%s' for performance tracking\n", thread_name);
    
    return thread_id;
}

int performance_thread_update(int thread_perf_id, uint64_t operations_done, uint64_t bytes_processed) {
    if (!g_performance_monitor.initialized || thread_perf_id < 0 || 
        thread_perf_id >= g_performance_monitor.active_thread_count) return -1;
    
    pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
    
    thread_performance_t* thread_perf = &g_performance_monitor.threads[thread_perf_id];
    if (thread_perf->active) {
        thread_perf->operations_count += operations_done;
        thread_perf->bytes_processed += bytes_processed;
        
        // Update global counters
        g_performance_monitor.total_operations += operations_done;
        g_performance_monitor.total_bytes_processed += bytes_processed;
        
        // Calculate throughput
        uint64_t elapsed_ns = get_timestamp_ns() - g_performance_monitor.system_start_time;
        if (elapsed_ns > 0) {
            g_performance_monitor.overall_throughput = 
                (double)g_performance_monitor.total_operations / (elapsed_ns / 1000000000.0);
        }
    }
    
    pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
    
    return 0;
}

// Statistics and reporting
int performance_get_statistics(performance_category_t category, bool include_histograms, char** stats_json) {
    if (!g_performance_monitor.initialized || !stats_json) return -1;
    
    pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
    
    json_object* stats = json_object_new_object();
    
    // System info
    json_object_object_add(stats, "monitoring_enabled", json_object_new_boolean(g_performance_monitor.monitoring_enabled));
    json_object_object_add(stats, "profiling_enabled", json_object_new_boolean(g_performance_monitor.profiling_enabled));
    json_object_object_add(stats, "sample_interval_ms", json_object_new_int(g_performance_monitor.sample_interval_ms));
    json_object_object_add(stats, "total_operations", json_object_new_int64(g_performance_monitor.total_operations));
    json_object_object_add(stats, "total_bytes_processed", json_object_new_int64(g_performance_monitor.total_bytes_processed));
    json_object_object_add(stats, "overall_throughput_ops_sec", json_object_new_double(g_performance_monitor.overall_throughput));
    
    uint64_t uptime_ns = get_timestamp_ns() - g_performance_monitor.system_start_time;
    json_object_object_add(stats, "uptime_seconds", json_object_new_double(uptime_ns / 1000000000.0));
    
    // Counters
    json_object* counters_array = json_object_new_array();
    for (int i = 0; i < g_performance_monitor.active_counter_count; i++) {
        performance_counter_t* counter = &g_performance_monitor.counters[i];
        if (counter->active && (category == PERF_CATEGORY_COUNT || counter->category == category)) {
            json_object* counter_obj = json_object_new_object();
            json_object_object_add(counter_obj, "name", json_object_new_string(counter->name));
            json_object_object_add(counter_obj, "category", json_object_new_string(performance_category_to_string(counter->category)));
            json_object_object_add(counter_obj, "value", json_object_new_int64(counter->value));
            json_object_object_add(counter_obj, "total", json_object_new_int64(counter->total));
            json_object_object_add(counter_obj, "average", json_object_new_double(counter->average));
            json_object_object_add(counter_obj, "min_value", json_object_new_int64(counter->min_value));
            json_object_object_add(counter_obj, "max_value", json_object_new_int64(counter->max_value));
            json_object_object_add(counter_obj, "sample_count", json_object_new_int64(counter->sample_count));
            
            json_object_array_add(counters_array, counter_obj);
        }
    }
    json_object_object_add(stats, "counters", counters_array);
    
    // Timers
    json_object* timers_array = json_object_new_array();
    for (int i = 0; i < g_performance_monitor.active_timer_count; i++) {
        performance_timer_t* timer = &g_performance_monitor.timers[i];
        if (timer->active && (category == PERF_CATEGORY_COUNT || timer->category == category)) {
            json_object* timer_obj = json_object_new_object();
            json_object_object_add(timer_obj, "name", json_object_new_string(timer->name));
            json_object_object_add(timer_obj, "category", json_object_new_string(performance_category_to_string(timer->category)));
            json_object_object_add(timer_obj, "total_time_ns", json_object_new_int64(timer->total_time));
            json_object_object_add(timer_obj, "avg_time_ns", json_object_new_double(timer->avg_time));
            json_object_object_add(timer_obj, "min_time_ns", json_object_new_int64(timer->min_time));
            json_object_object_add(timer_obj, "max_time_ns", json_object_new_int64(timer->max_time));
            json_object_object_add(timer_obj, "call_count", json_object_new_int64(timer->call_count));
            json_object_object_add(timer_obj, "active_timers", json_object_new_int64(timer->active_timers));
            
            json_object_array_add(timers_array, timer_obj);
        }
    }
    json_object_object_add(stats, "timers", timers_array);
    
    // System resources
    json_object* resources_obj = json_object_new_object();
    json_object_object_add(resources_obj, "memory_rss_bytes", json_object_new_int64(g_performance_monitor.current_resources.memory_rss_bytes));
    json_object_object_add(resources_obj, "memory_vms_bytes", json_object_new_int64(g_performance_monitor.current_resources.memory_vms_bytes));
    json_object_object_add(resources_obj, "cpu_usage_percent", json_object_new_double(g_performance_monitor.current_resources.cpu_usage_percent));
    json_object_object_add(resources_obj, "thread_count", json_object_new_int(g_performance_monitor.current_resources.thread_count));
    json_object_object_add(resources_obj, "load_average_1m", json_object_new_double(g_performance_monitor.current_resources.load_average_1m));
    json_object_object_add(resources_obj, "open_file_descriptors", json_object_new_int(g_performance_monitor.current_resources.open_file_descriptors));
    json_object_object_add(stats, "system_resources", resources_obj);
    
    // Peak resources
    json_object* peak_resources_obj = json_object_new_object();
    json_object_object_add(peak_resources_obj, "peak_memory_rss_bytes", json_object_new_int64(g_performance_monitor.peak_resources.memory_rss_bytes));
    json_object_object_add(peak_resources_obj, "peak_cpu_usage_percent", json_object_new_double(g_performance_monitor.peak_resources.cpu_usage_percent));
    json_object_object_add(stats, "peak_resources", peak_resources_obj);
    
    pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
    
    const char* json_str = json_object_to_json_string_ext(stats, JSON_C_TO_STRING_PRETTY);
    *stats_json = strdup(json_str);
    json_object_put(stats);
    
    return 0;
}

// Get current system resource utilization
int performance_get_system_resources(system_resources_t* resources) {
    if (!resources) return -1;
    
    if (!g_performance_monitor.initialized) {
        memset(resources, 0, sizeof(system_resources_t));
        return -1;
    }
    
    pthread_mutex_lock(&g_performance_monitor.monitor_mutex);
    *resources = g_performance_monitor.current_resources;
    pthread_mutex_unlock(&g_performance_monitor.monitor_mutex);
    
    return 0;
}