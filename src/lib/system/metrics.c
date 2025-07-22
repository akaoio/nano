#define _DEFAULT_SOURCE
#include "metrics.h"
#include "../common/time_utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static metrics_system_t g_metrics = {0};

static metric_t* metrics_find_or_create(const char* name, const char* labels[][2], int label_count) {
    // Find existing metric
    for (size_t i = 0; i < g_metrics.metric_count; i++) {
        metric_t* metric = &g_metrics.metrics[i];
        
        if (strcmp(metric->name, name) == 0 && metric->label_count == label_count) {
            // Check if labels match
            bool labels_match = true;
            for (int j = 0; j < label_count; j++) {
                if (strcmp(metric->labels[j][0], labels[j][0]) != 0 ||
                    strcmp(metric->labels[j][1], labels[j][1]) != 0) {
                    labels_match = false;
                    break;
                }
            }
            
            if (labels_match) {
                return metric;
            }
        }
    }
    
    // Create new metric
    if (g_metrics.metric_count >= g_metrics.max_metrics) {
        return NULL; // No space
    }
    
    metric_t* metric = &g_metrics.metrics[g_metrics.metric_count++];
    memset(metric, 0, sizeof(metric_t));
    
    strncpy(metric->name, name, sizeof(metric->name) - 1);
    metric->timestamp = get_timestamp_ms();
    metric->label_count = label_count;
    
    // Copy labels
    for (int i = 0; i < label_count && i < METRICS_MAX_LABEL_PAIRS; i++) {
        strncpy(metric->labels[i][0], labels[i][0], sizeof(metric->labels[i][0]) - 1);
        strncpy(metric->labels[i][1], labels[i][1], sizeof(metric->labels[i][1]) - 1);
    }
    
    return metric;
}

int metrics_init(void) {
    if (g_metrics.initialized) {
        return 0; // Already initialized
    }
    
    g_metrics.max_metrics = 1000;
    g_metrics.metrics = calloc(g_metrics.max_metrics, sizeof(metric_t));
    if (!g_metrics.metrics) {
        return -1;
    }
    
    if (pthread_mutex_init(&g_metrics.metrics_mutex, NULL) != 0) {
        free(g_metrics.metrics);
        return -1;
    }
    
    g_metrics.start_time = get_timestamp_ms();
    g_metrics.initialized = true;
    g_metrics.total_metric_updates = 0;
    
    // Initialize core metrics
    metrics_counter_inc("mcp_server_starts_total", NULL, 0);
    metrics_gauge_set("mcp_server_start_timestamp", (double)g_metrics.start_time, NULL, 0);
    
    return 0;
}

void metrics_shutdown(void) {
    if (!g_metrics.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    free(g_metrics.metrics);
    g_metrics.metrics = NULL;
    g_metrics.metric_count = 0;
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
    
    pthread_mutex_destroy(&g_metrics.metrics_mutex);
    g_metrics.initialized = false;
}

bool metrics_is_initialized(void) {
    return g_metrics.initialized;
}

void metrics_counter_inc(const char* name, const char* labels[][2], int label_count) {
    metrics_counter_add(name, 1.0, labels, label_count);
}

void metrics_counter_add(const char* name, double value, const char* labels[][2], int label_count) {
    if (!g_metrics.initialized || value < 0) {
        return;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    metric_t* metric = metrics_find_or_create(name, labels, label_count);
    if (metric) {
        metric->value += value;
        metric->timestamp = get_timestamp_ms();
        g_metrics.total_metric_updates++;
    }
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
}

void metrics_gauge_set(const char* name, double value, const char* labels[][2], int label_count) {
    if (!g_metrics.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    metric_t* metric = metrics_find_or_create(name, labels, label_count);
    if (metric) {
        metric->value = value;
        metric->timestamp = get_timestamp_ms();
        g_metrics.total_metric_updates++;
    }
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
}

void metrics_gauge_inc(const char* name, const char* labels[][2], int label_count) {
    if (!g_metrics.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    metric_t* metric = metrics_find_or_create(name, labels, label_count);
    if (metric) {
        metric->value += 1.0;
        metric->timestamp = get_timestamp_ms();
        g_metrics.total_metric_updates++;
    }
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
}

void metrics_gauge_dec(const char* name, const char* labels[][2], int label_count) {
    if (!g_metrics.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    metric_t* metric = metrics_find_or_create(name, labels, label_count);
    if (metric) {
        metric->value -= 1.0;
        metric->timestamp = get_timestamp_ms();
        g_metrics.total_metric_updates++;
    }
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
}

void metrics_histogram_observe(const char* name, double value, const char* labels[][2], int label_count) {
    if (!g_metrics.initialized) {
        return;
    }
    
    // Simple histogram implementation - just track count and sum
    char count_name[METRICS_MAX_NAME_LEN], sum_name[METRICS_MAX_NAME_LEN];
    snprintf(count_name, sizeof(count_name), "%s_count", name);
    snprintf(sum_name, sizeof(sum_name), "%s_sum", name);
    
    metrics_counter_inc(count_name, labels, label_count);
    metrics_counter_add(sum_name, value, labels, label_count);
}

int metrics_export_prometheus(char* output, size_t output_size) {
    if (!g_metrics.initialized || !output || output_size == 0) {
        return -1;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    size_t written = 0;
    output[0] = '\0';
    
    for (size_t i = 0; i < g_metrics.metric_count; i++) {
        metric_t* metric = &g_metrics.metrics[i];
        char line[512];
        
        if (metric->label_count > 0) {
            char labels[256] = {0};
            for (int j = 0; j < metric->label_count; j++) {
                if (j > 0) strcat(labels, ",");
                strcat(labels, metric->labels[j][0]);
                strcat(labels, "=\"");
                strcat(labels, metric->labels[j][1]);
                strcat(labels, "\"");
            }
            snprintf(line, sizeof(line), "%s{%s} %.2f %lu\n",
                    metric->name, labels, metric->value, metric->timestamp);
        } else {
            snprintf(line, sizeof(line), "%s %.2f %lu\n",
                    metric->name, metric->value, metric->timestamp);
        }
        
        size_t line_len = strlen(line);
        if (written + line_len >= output_size - 1) {
            break; // Buffer full
        }
        
        strcat(output, line);
        written += line_len;
    }
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
    
    return (int)written;
}

int metrics_get_system_stats(size_t* metric_count, uint64_t* total_updates, uint64_t* uptime_ms) {
    if (!g_metrics.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    if (metric_count) *metric_count = g_metrics.metric_count;
    if (total_updates) *total_updates = g_metrics.total_metric_updates;
    if (uptime_ms) *uptime_ms = get_timestamp_ms() - g_metrics.start_time;
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
    
    return 0;
}

void metrics_clear_all(void) {
    if (!g_metrics.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    g_metrics.metric_count = 0;
    g_metrics.total_metric_updates = 0;
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
}

int metrics_get_value(const char* name, const char* labels[][2], int label_count, double* value) {
    if (!g_metrics.initialized || !value) {
        return -1;
    }
    
    pthread_mutex_lock(&g_metrics.metrics_mutex);
    
    metric_t* metric = metrics_find_or_create(name, labels, label_count);
    if (metric) {
        *value = metric->value;
        pthread_mutex_unlock(&g_metrics.metrics_mutex);
        return 0;
    }
    
    pthread_mutex_unlock(&g_metrics.metrics_mutex);
    return -1; // Not found
}

// Standard server metrics - convenience functions
void metrics_request_received(const char* transport, const char* method) {
    const char* labels[][2] = {
        {"transport", transport},
        {"method", method}
    };
    metrics_counter_inc("mcp_requests_total", labels, 2);
}

void metrics_request_completed(const char* transport, const char* method, double duration_ms) {
    const char* labels[][2] = {
        {"transport", transport},
        {"method", method}
    };
    metrics_counter_inc("mcp_requests_completed_total", labels, 2);
    metrics_histogram_observe("mcp_request_duration_seconds", duration_ms / 1000.0, labels, 2);
}

void metrics_request_failed(const char* transport, const char* method, const char* error_type) {
    const char* labels[][2] = {
        {"transport", transport},
        {"method", method},
        {"error", error_type}
    };
    metrics_counter_inc("mcp_requests_failed_total", labels, 3);
}

void metrics_connection_opened(const char* transport) {
    const char* labels[][2] = {
        {"transport", transport}
    };
    metrics_counter_inc("mcp_connections_opened_total", labels, 1);
    metrics_gauge_inc("mcp_active_connections", labels, 1);
}

void metrics_connection_closed(const char* transport) {
    const char* labels[][2] = {
        {"transport", transport}
    };
    metrics_counter_inc("mcp_connections_closed_total", labels, 1);
    metrics_gauge_dec("mcp_active_connections", labels, 1);
}

void metrics_memory_usage(size_t bytes) {
    metrics_gauge_set("mcp_memory_usage_bytes", (double)bytes, NULL, 0);
}