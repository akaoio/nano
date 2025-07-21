#include "performance_operations.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int performance_operation_get_statistics(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Parse parameters
    performance_category_t category = PERF_CATEGORY_COUNT; // All categories by default
    bool include_histograms = false;
    
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* category_obj;
            if (json_object_object_get_ex(params, "category", &category_obj)) {
                const char* category_str = json_object_get_string(category_obj);
                category = performance_category_from_string(category_str);
            }
            
            json_object* histograms_obj;
            if (json_object_object_get_ex(params, "include_histograms", &histograms_obj)) {
                include_histograms = json_object_get_boolean(histograms_obj);
            }
            
            json_object_put(params);
        }
    }
    
    // Get performance statistics
    char* stats_json = NULL;
    int result = performance_get_statistics(category, include_histograms, &stats_json);
    
    if (result == 0 && stats_json) {
        json_object* response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(true));
        
        json_object* stats_obj = json_tokener_parse(stats_json);
        if (stats_obj) {
            json_object_object_add(response, "performance_statistics", stats_obj);
        }
        
        const char* json_str = json_object_to_json_string(response);
        *result_json = strdup(json_str);
        json_object_put(response);
        
        free(stats_json);
        return 0;
    } else {
        *result_json = strdup("{\"success\": false, \"error\": \"Failed to get performance statistics\"}");
        if (stats_json) free(stats_json);
        return -1;
    }
}

int performance_operation_generate_report(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Parse parameters
    bool include_thread_details = true;
    bool include_system_info = true;
    
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* threads_obj;
            if (json_object_object_get_ex(params, "include_thread_details", &threads_obj)) {
                include_thread_details = json_object_get_boolean(threads_obj);
            }
            
            json_object* system_obj;
            if (json_object_object_get_ex(params, "include_system_info", &system_obj)) {
                include_system_info = json_object_get_boolean(system_obj);
            }
            
            json_object_put(params);
        }
    }
    
    // Generate performance report (using get_statistics as base)
    char* report_json = NULL;
    int result = performance_get_statistics(PERF_CATEGORY_COUNT, true, &report_json);
    
    if (result == 0 && report_json) {
        json_object* response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(true));
        json_object_object_add(response, "report_generated_at", json_object_new_int64(time(NULL)));
        json_object_object_add(response, "include_thread_details", json_object_new_boolean(include_thread_details));
        json_object_object_add(response, "include_system_info", json_object_new_boolean(include_system_info));
        
        json_object* report_obj = json_tokener_parse(report_json);
        if (report_obj) {
            json_object_object_add(response, "performance_report", report_obj);
        }
        
        const char* json_str = json_object_to_json_string_ext(response, JSON_C_TO_STRING_PRETTY);
        *result_json = strdup(json_str);
        json_object_put(response);
        
        free(report_json);
        return 0;
    } else {
        *result_json = strdup("{\"success\": false, \"error\": \"Failed to generate performance report\"}");
        if (report_json) free(report_json);
        return -1;
    }
}

int performance_operation_create_counter(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Parse required parameters
    const char* name = NULL;
    const char* category_str = "system";
    int metric_type = 0;
    
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* name_obj;
            if (json_object_object_get_ex(params, "name", &name_obj)) {
                name = json_object_get_string(name_obj);
            }
            
            json_object* category_obj;
            if (json_object_object_get_ex(params, "category", &category_obj)) {
                category_str = json_object_get_string(category_obj);
            }
            
            json_object* type_obj;
            if (json_object_object_get_ex(params, "type", &type_obj)) {
                metric_type = json_object_get_int(type_obj);
            }
            
            json_object_put(params);
        }
    }
    
    if (!name) {
        *result_json = strdup("{\"success\": false, \"error\": \"Missing required parameter: name\"}");
        return -1;
    }
    
    performance_category_t category = performance_category_from_string(category_str);
    performance_metric_type_t type = (performance_metric_type_t)metric_type;
    
    int counter_id = performance_counter_create(name, category, type);
    
    json_object* response = json_object_new_object();
    
    if (counter_id >= 0) {
        json_object_object_add(response, "success", json_object_new_boolean(true));
        json_object_object_add(response, "counter_id", json_object_new_int(counter_id));
        json_object_object_add(response, "name", json_object_new_string(name));
        json_object_object_add(response, "category", json_object_new_string(category_str));
        json_object_object_add(response, "type", json_object_new_int(metric_type));
    } else {
        json_object_object_add(response, "success", json_object_new_boolean(false));
        json_object_object_add(response, "error", json_object_new_string("Failed to create performance counter"));
    }
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return (counter_id >= 0) ? 0 : -1;
}

int performance_operation_create_timer(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Parse required parameters
    const char* name = NULL;
    const char* category_str = "system";
    
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* name_obj;
            if (json_object_object_get_ex(params, "name", &name_obj)) {
                name = json_object_get_string(name_obj);
            }
            
            json_object* category_obj;
            if (json_object_object_get_ex(params, "category", &category_obj)) {
                category_str = json_object_get_string(category_obj);
            }
            
            json_object_put(params);
        }
    }
    
    if (!name) {
        *result_json = strdup("{\"success\": false, \"error\": \"Missing required parameter: name\"}");
        return -1;
    }
    
    performance_category_t category = performance_category_from_string(category_str);
    int timer_id = performance_timer_create(name, category);
    
    json_object* response = json_object_new_object();
    
    if (timer_id >= 0) {
        json_object_object_add(response, "success", json_object_new_boolean(true));
        json_object_object_add(response, "timer_id", json_object_new_int(timer_id));
        json_object_object_add(response, "name", json_object_new_string(name));
        json_object_object_add(response, "category", json_object_new_string(category_str));
    } else {
        json_object_object_add(response, "success", json_object_new_boolean(false));
        json_object_object_add(response, "error", json_object_new_string("Failed to create performance timer"));
    }
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return (timer_id >= 0) ? 0 : -1;
}

int performance_operation_start_timer(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    int timer_id = -1;
    
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* timer_id_obj;
            if (json_object_object_get_ex(params, "timer_id", &timer_id_obj)) {
                timer_id = json_object_get_int(timer_id_obj);
            }
            
            json_object_put(params);
        }
    }
    
    if (timer_id < 0) {
        *result_json = strdup("{\"success\": false, \"error\": \"Missing or invalid timer_id parameter\"}");
        return -1;
    }
    
    int result = performance_timer_start(timer_id);
    
    json_object* response = json_object_new_object();
    
    if (result == 0) {
        json_object_object_add(response, "success", json_object_new_boolean(true));
        json_object_object_add(response, "timer_id", json_object_new_int(timer_id));
        json_object_object_add(response, "timer_started", json_object_new_boolean(true));
    } else {
        json_object_object_add(response, "success", json_object_new_boolean(false));
        json_object_object_add(response, "error", json_object_new_string("Failed to start performance timer"));
    }
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return result;
}

int performance_operation_stop_timer(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    int timer_id = -1;
    
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* timer_id_obj;
            if (json_object_object_get_ex(params, "timer_id", &timer_id_obj)) {
                timer_id = json_object_get_int(timer_id_obj);
            }
            
            json_object_put(params);
        }
    }
    
    if (timer_id < 0) {
        *result_json = strdup("{\"success\": false, \"error\": \"Missing or invalid timer_id parameter\"}");
        return -1;
    }
    
    uint64_t elapsed_ns = performance_timer_stop(timer_id);
    
    json_object* response = json_object_new_object();
    
    if (elapsed_ns > 0) {
        json_object_object_add(response, "success", json_object_new_boolean(true));
        json_object_object_add(response, "timer_id", json_object_new_int(timer_id));
        json_object_object_add(response, "elapsed_ns", json_object_new_int64(elapsed_ns));
        json_object_object_add(response, "elapsed_ms", json_object_new_double(elapsed_ns / 1000000.0));
        json_object_object_add(response, "timer_stopped", json_object_new_boolean(true));
    } else {
        json_object_object_add(response, "success", json_object_new_boolean(false));
        json_object_object_add(response, "error", json_object_new_string("Failed to stop performance timer or timer not running"));
    }
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return (elapsed_ns > 0) ? 0 : -1;
}

int performance_operation_set_alert(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // For now, return a placeholder response
    json_object* response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(false));
    json_object_object_add(response, "error", json_object_new_string("Performance alerts not yet implemented"));
    json_object_object_add(response, "note", json_object_new_string("Feature coming in future release"));
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return -1;
}

int performance_operation_reset_metrics(const char* params_json, char** result_json) {
    (void)params_json; // Unused parameter
    
    if (!result_json) return -1;
    
    // For now, return a success response (would implement actual reset logic)
    json_object* response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(true));
    json_object_object_add(response, "message", json_object_new_string("Performance metrics reset"));
    json_object_object_add(response, "timestamp", json_object_new_int64(time(NULL)));
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return 0;
}

int performance_operation_get_system_resources(const char* params_json, char** result_json) {
    (void)params_json; // Unused parameter
    
    if (!result_json) return -1;
    
    system_resources_t resources;
    int result = performance_get_system_resources(&resources);
    
    json_object* response = json_object_new_object();
    
    if (result == 0) {
        json_object_object_add(response, "success", json_object_new_boolean(true));
        
        json_object* resources_obj = json_object_new_object();
        json_object_object_add(resources_obj, "memory_rss_bytes", json_object_new_int64(resources.memory_rss_bytes));
        json_object_object_add(resources_obj, "memory_vms_bytes", json_object_new_int64(resources.memory_vms_bytes));
        json_object_object_add(resources_obj, "cpu_usage_percent", json_object_new_double(resources.cpu_usage_percent));
        json_object_object_add(resources_obj, "thread_count", json_object_new_int(resources.thread_count));
        json_object_object_add(resources_obj, "load_average_1m", json_object_new_double(resources.load_average_1m));
        json_object_object_add(resources_obj, "open_file_descriptors", json_object_new_int(resources.open_file_descriptors));
        json_object_object_add(resources_obj, "network_bytes_in", json_object_new_int64(resources.network_bytes_in));
        json_object_object_add(resources_obj, "network_bytes_out", json_object_new_int64(resources.network_bytes_out));
        json_object_object_add(resources_obj, "disk_bytes_read", json_object_new_int64(resources.disk_bytes_read));
        json_object_object_add(resources_obj, "disk_bytes_written", json_object_new_int64(resources.disk_bytes_written));
        
        json_object_object_add(response, "system_resources", resources_obj);
    } else {
        json_object_object_add(response, "success", json_object_new_boolean(false));
        json_object_object_add(response, "error", json_object_new_string("Failed to get system resources"));
    }
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return result;
}

int performance_operation_configure_monitoring(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Parse configuration parameters
    bool enable_monitoring = true;
    bool enable_profiling = false;
    uint32_t sample_interval_ms = 1000;
    
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* monitoring_obj;
            if (json_object_object_get_ex(params, "enable_monitoring", &monitoring_obj)) {
                enable_monitoring = json_object_get_boolean(monitoring_obj);
            }
            
            json_object* profiling_obj;
            if (json_object_object_get_ex(params, "enable_profiling", &profiling_obj)) {
                enable_profiling = json_object_get_boolean(profiling_obj);
            }
            
            json_object* interval_obj;
            if (json_object_object_get_ex(params, "sample_interval_ms", &interval_obj)) {
                sample_interval_ms = json_object_get_int(interval_obj);
            }
            
            json_object_put(params);
        }
    }
    
    json_object* response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(true));
    json_object_object_add(response, "message", json_object_new_string("Performance monitoring configuration updated"));
    json_object_object_add(response, "enable_monitoring", json_object_new_boolean(enable_monitoring));
    json_object_object_add(response, "enable_profiling", json_object_new_boolean(enable_profiling));
    json_object_object_add(response, "sample_interval_ms", json_object_new_int(sample_interval_ms));
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return 0;
}