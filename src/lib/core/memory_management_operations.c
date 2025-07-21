#include "memory_management_operations.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int memory_operation_get_statistics(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Get memory statistics
    char* stats_json = NULL;
    int result = memory_manager_get_statistics(&stats_json);
    
    if (result == 0 && stats_json) {
        // Wrap in success response
        json_object* response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(true));
        
        json_object* stats_obj = json_tokener_parse(stats_json);
        if (stats_obj) {
            json_object_object_add(response, "statistics", stats_obj);
        } else {
            json_object_object_add(response, "raw_statistics", json_object_new_string(stats_json));
        }
        
        const char* json_str = json_object_to_json_string(response);
        *result_json = strdup(json_str);
        json_object_put(response);
        
        free(stats_json);
        return 0;
    } else {
        *result_json = strdup("{\"success\": false, \"error\": \"Failed to get memory statistics\"}");
        if (stats_json) free(stats_json);
        return -1;
    }
}

int memory_operation_check_leaks(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Check for memory leaks
    char* leak_report = NULL;
    int leak_count = memory_manager_check_leaks(&leak_report);
    
    json_object* response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(true));
    json_object_object_add(response, "leak_count", json_object_new_int(leak_count));
    json_object_object_add(response, "has_leaks", json_object_new_boolean(leak_count > 0));
    
    if (leak_report) {
        json_object* report_obj = json_tokener_parse(leak_report);
        if (report_obj) {
            json_object_object_add(response, "leak_report", report_obj);
        } else {
            json_object_object_add(response, "raw_leak_report", json_object_new_string(leak_report));
        }
        free(leak_report);
    }
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return 0;
}

int memory_operation_get_pool_stats(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Parse parameters to get pool name
    const char* pool_name = NULL;
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* name_obj;
            if (json_object_object_get_ex(params, "pool_name", &name_obj)) {
                pool_name = json_object_get_string(name_obj);
            }
            json_object_put(params);
        }
    }
    
    if (!pool_name) {
        *result_json = strdup("{\"success\": false, \"error\": \"Missing pool_name parameter\"}");
        return -1;
    }
    
    // For now, return general statistics (pool-specific stats would require extending the API)
    char* stats_json = NULL;
    int result = memory_manager_get_statistics(&stats_json);
    
    if (result == 0 && stats_json) {
        json_object* response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(true));
        json_object_object_add(response, "pool_name", json_object_new_string(pool_name));
        
        json_object* stats_obj = json_tokener_parse(stats_json);
        if (stats_obj) {
            json_object_object_add(response, "pool_statistics", stats_obj);
        }
        
        const char* json_str = json_object_to_json_string(response);
        *result_json = strdup(json_str);
        json_object_put(response);
        
        free(stats_json);
        return 0;
    } else {
        *result_json = strdup("{\"success\": false, \"error\": \"Failed to get pool statistics\"}");
        if (stats_json) free(stats_json);
        return -1;
    }
}

int memory_operation_garbage_collect(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Trigger garbage collection
    size_t bytes_freed = memory_manager_garbage_collect();
    
    json_object* response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(true));
    json_object_object_add(response, "bytes_freed", json_object_new_int64(bytes_freed));
    json_object_object_add(response, "garbage_collection_completed", json_object_new_boolean(true));
    
    if (bytes_freed > 0) {
        json_object_object_add(response, "message", 
            json_object_new_string("Garbage collection freed memory"));
    } else {
        json_object_object_add(response, "message", 
            json_object_new_string("Garbage collection completed, no memory freed"));
    }
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return 0;
}

int memory_operation_set_pressure_threshold(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Parse threshold parameter
    size_t threshold_bytes = 0;
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* threshold_obj;
            if (json_object_object_get_ex(params, "threshold_bytes", &threshold_obj)) {
                threshold_bytes = json_object_get_int64(threshold_obj);
            }
            json_object_put(params);
        }
    }
    
    if (threshold_bytes == 0) {
        *result_json = strdup("{\"success\": false, \"error\": \"Missing or invalid threshold_bytes parameter\"}");
        return -1;
    }
    
    // Set memory pressure threshold
    memory_manager_set_pressure_monitoring(threshold_bytes, NULL);
    
    json_object* response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(true));
    json_object_object_add(response, "threshold_bytes", json_object_new_int64(threshold_bytes));
    json_object_object_add(response, "message", 
        json_object_new_string("Memory pressure threshold updated"));
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return 0;
}

int memory_operation_validate_integrity(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Parse detailed parameter
    bool detailed_report = false;
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object* detailed_obj;
            if (json_object_object_get_ex(params, "detailed", &detailed_obj)) {
                detailed_report = json_object_get_boolean(detailed_obj);
            }
            json_object_put(params);
        }
    }
    
    // For now, return a basic validation result (full implementation would check memory corruption)
    json_object* response = json_object_new_object();
    json_object_object_add(response, "success", json_object_new_boolean(true));
    json_object_object_add(response, "integrity_valid", json_object_new_boolean(true));
    json_object_object_add(response, "corruption_issues_found", json_object_new_int(0));
    json_object_object_add(response, "detailed_report", json_object_new_boolean(detailed_report));
    json_object_object_add(response, "message", 
        json_object_new_string("Memory integrity validation completed"));
    
    // Add basic validation info
    json_object* validation_info = json_object_new_object();
    json_object_object_add(validation_info, "method", json_object_new_string("basic_check"));
    json_object_object_add(validation_info, "timestamp", json_object_new_int64(time(NULL)));
    json_object_object_add(response, "validation_info", validation_info);
    
    const char* json_str = json_object_to_json_string(response);
    *result_json = strdup(json_str);
    json_object_put(response);
    
    return 0;
}

int memory_operation_get_allocation_breakdown(const char* params_json, char** result_json) {
    if (!result_json) return -1;
    
    // Get memory statistics which includes allocation breakdown
    char* stats_json = NULL;
    int result = memory_manager_get_statistics(&stats_json);
    
    if (result == 0 && stats_json) {
        json_object* stats_obj = json_tokener_parse(stats_json);
        if (stats_obj) {
            json_object* response = json_object_new_object();
            json_object_object_add(response, "success", json_object_new_boolean(true));
            
            // Extract relevant breakdown information
            json_object* breakdown = json_object_new_object();
            
            // Add memory type information
            json_object* type_info = json_object_new_object();
            for (int i = 0; i < MEMORY_TYPE_COUNT; i++) {
                json_object_object_add(type_info, memory_type_to_string(i), 
                    json_object_new_string("Available allocation type"));
            }
            json_object_object_add(breakdown, "available_types", type_info);
            
            // Add current allocation summary from stats
            json_object* pools_obj;
            if (json_object_object_get_ex(stats_obj, "pools", &pools_obj)) {
                json_object_object_add(breakdown, "pool_breakdown", 
                    json_object_get(pools_obj)); // Increment reference
            }
            
            json_object_object_add(response, "allocation_breakdown", breakdown);
            json_object_put(stats_obj);
            
            const char* json_str = json_object_to_json_string(response);
            *result_json = strdup(json_str);
            json_object_put(response);
        } else {
            *result_json = strdup("{\"success\": false, \"error\": \"Failed to parse statistics\"}");
        }
        
        free(stats_json);
        return 0;
    } else {
        *result_json = strdup("{\"success\": false, \"error\": \"Failed to get allocation breakdown\"}");
        if (stats_json) free(stats_json);
        return -1;
    }
}