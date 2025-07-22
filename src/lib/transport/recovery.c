#define _DEFAULT_SOURCE
#include "recovery.h"
#include "manager.h"
#include "common/time_utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static recovery_manager_t g_recovery_manager = {0};

// Forward declaration
static void* transport_recovery_thread(void* arg);

// Helper function to convert transport type to string
static const char* transport_type_to_string(transport_type_t type) {
    switch (type) {
        case TRANSPORT_TYPE_STDIO: return "stdio";
        case TRANSPORT_TYPE_HTTP: return "http";
        case TRANSPORT_TYPE_WEBSOCKET: return "websocket";
        case TRANSPORT_TYPE_TCP: return "tcp";
        case TRANSPORT_TYPE_UDP: return "udp";
        default: return "unknown";
    }
}

static const char* failure_type_to_string(failure_type_t type) {
    switch (type) {
        case FAILURE_TYPE_CONNECTION_LOST: return "Connection Lost";
        case FAILURE_TYPE_TIMEOUT: return "Timeout";
        case FAILURE_TYPE_RESOURCE_BUSY: return "Resource Busy";
        case FAILURE_TYPE_PERMISSION_DENIED: return "Permission Denied";
        case FAILURE_TYPE_PORT_CONFLICT: return "Port Conflict";
        case FAILURE_TYPE_NETWORK_ERROR: return "Network Error";
        case FAILURE_TYPE_UNKNOWN:
        default: return "Unknown";
    }
}

static const char* recovery_state_to_string(recovery_state_t state) {
    switch (state) {
        case RECOVERY_STATE_IDLE: return "Idle";
        case RECOVERY_STATE_ACTIVE: return "Active";
        case RECOVERY_STATE_FAILED: return "Failed";
        case RECOVERY_STATE_SUCCESS: return "Success";
        default: return "Unknown";
    }
}

static bool is_recoverable_failure(failure_type_t failure_type) {
    switch (failure_type) {
        case FAILURE_TYPE_CONNECTION_LOST:
        case FAILURE_TYPE_TIMEOUT:
        case FAILURE_TYPE_RESOURCE_BUSY:
        case FAILURE_TYPE_NETWORK_ERROR:
            return true;
            
        case FAILURE_TYPE_PERMISSION_DENIED:
        case FAILURE_TYPE_PORT_CONFLICT:
        case FAILURE_TYPE_UNKNOWN:
        default:
            return false;
    }
}

static int calculate_retry_interval(transport_recovery_t* recovery) {
    int base_interval = recovery->retry_interval_ms;
    int multiplier = 1;
    
    // Exponential backoff: 1x, 2x, 4x, 8x, etc.
    for (int i = 0; i < recovery->failure_count - 1; i++) {
        multiplier *= recovery->backoff_multiplier;
    }
    
    int interval = base_interval * multiplier;
    return (interval > recovery->max_retry_interval_ms) ? 
           recovery->max_retry_interval_ms : interval;
}

int transport_recovery_init(bool auto_recovery) {
    if (g_recovery_manager.initialized) return 0;
    
    if (pthread_mutex_init(&g_recovery_manager.recovery_mutex, NULL) != 0) {
        return -1;
    }
    
    // Initialize recovery settings for each transport
    for (int i = 0; i < TRANSPORT_TYPE_COUNT; i++) {
        transport_recovery_t* recovery = &g_recovery_manager.transports[i];
        recovery->transport_type = i;
        recovery->failure_type = FAILURE_TYPE_UNKNOWN;
        recovery->connection_lost = false;
        recovery->failure_count = 0;
        recovery->max_retries = 5;
        recovery->retry_interval_ms = 1000;  // 1 second
        recovery->backoff_multiplier = 2;
        recovery->max_retry_interval_ms = 30000;  // 30 seconds
        recovery->state = RECOVERY_STATE_IDLE;
        recovery->shutdown_requested = false;
        recovery->successful_recoveries = 0;
        memset(recovery->last_error_message, 0, sizeof(recovery->last_error_message));
    }
    
    g_recovery_manager.auto_recovery_enabled = auto_recovery;
    g_recovery_manager.log_file = fopen("transport_recovery.log", "w");
    if (!g_recovery_manager.log_file) {
        fprintf(stderr, "Warning: Could not open transport recovery log file\n");
    }
    
    g_recovery_manager.initialized = true;
    
    if (g_recovery_manager.log_file) {
        fprintf(g_recovery_manager.log_file, 
                "[%llu] Transport recovery system initialized (auto_recovery: %s)\n",
                (unsigned long long)get_timestamp_ms(),
                auto_recovery ? "enabled" : "disabled");
        fflush(g_recovery_manager.log_file);
    }
    
    return 0;
}

void transport_recovery_shutdown(void) {
    if (!g_recovery_manager.initialized) return;
    
    pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
    
    // Signal all recovery threads to shutdown
    for (int i = 0; i < TRANSPORT_TYPE_COUNT; i++) {
        transport_recovery_t* recovery = &g_recovery_manager.transports[i];
        if (recovery->state == RECOVERY_STATE_ACTIVE) {
            recovery->shutdown_requested = true;
        }
    }
    
    pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
    
    // Wait for all recovery threads to complete
    for (int i = 0; i < TRANSPORT_TYPE_COUNT; i++) {
        transport_recovery_t* recovery = &g_recovery_manager.transports[i];
        if (recovery->state == RECOVERY_STATE_ACTIVE) {
            pthread_join(recovery->recovery_thread, NULL);
        }
    }
    
    if (g_recovery_manager.log_file) {
        fprintf(g_recovery_manager.log_file, 
                "[%llu] Transport recovery system shutdown\n",
                (unsigned long long)get_timestamp_ms());
        fprintf(g_recovery_manager.log_file, 
                "Total failures handled: %llu\n", 
                (unsigned long long)g_recovery_manager.total_failures);
        fprintf(g_recovery_manager.log_file, 
                "Total recoveries successful: %llu\n", 
                (unsigned long long)g_recovery_manager.total_recoveries);
        fclose(g_recovery_manager.log_file);
    }
    
    pthread_mutex_destroy(&g_recovery_manager.recovery_mutex);
    g_recovery_manager.initialized = false;
}

void transport_recovery_handle_failure(transport_type_t transport_type, 
                                     failure_type_t failure_type,
                                     const char* error_message) {
    if (!g_recovery_manager.initialized || transport_type >= TRANSPORT_TYPE_COUNT) {
        return;
    }
    
    pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
    
    transport_recovery_t* recovery = &g_recovery_manager.transports[transport_type];
    uint64_t now = get_timestamp_ms();
    
    recovery->connection_lost = true;
    recovery->failure_type = failure_type;
    recovery->last_failure_time = now;
    recovery->failure_count++;
    g_recovery_manager.total_failures++;
    
    if (recovery->failure_count == 1) {
        recovery->first_failure_time = now;
    }
    
    // Store error message
    if (error_message) {
        strncpy(recovery->last_error_message, error_message, 
                sizeof(recovery->last_error_message) - 1);
        recovery->last_error_message[sizeof(recovery->last_error_message) - 1] = '\0';
    }
    
    if (g_recovery_manager.log_file) {
        fprintf(g_recovery_manager.log_file,
                "[%llu] FAILURE: Transport %d (%s) - %s (attempt %d) - %s\n",
                (unsigned long long)now,
                transport_type, transport_type_to_string(transport_type),
                failure_type_to_string(failure_type), recovery->failure_count,
                error_message ? error_message : "No details");
        fflush(g_recovery_manager.log_file);
    }
    
    // Start recovery if enabled and not already active
    if (g_recovery_manager.auto_recovery_enabled &&
        recovery->state != RECOVERY_STATE_ACTIVE &&
        recovery->failure_count <= recovery->max_retries &&
        is_recoverable_failure(failure_type)) {
        
        recovery->state = RECOVERY_STATE_ACTIVE;
        recovery->recovery_start_time = now;
        recovery->shutdown_requested = false;
        
        if (pthread_create(&recovery->recovery_thread, NULL, 
                          transport_recovery_thread, recovery) == 0) {
            pthread_detach(recovery->recovery_thread);
            
            if (g_recovery_manager.log_file) {
                fprintf(g_recovery_manager.log_file,
                        "[%llu] RECOVERY: Starting recovery for transport %d\n",
                        (unsigned long long)now, transport_type);
                fflush(g_recovery_manager.log_file);
            }
        } else {
            recovery->state = RECOVERY_STATE_FAILED;
            
            if (g_recovery_manager.log_file) {
                fprintf(g_recovery_manager.log_file,
                        "[%llu] ERROR: Failed to start recovery thread for transport %d\n",
                        (unsigned long long)now, transport_type);
                fflush(g_recovery_manager.log_file);
            }
        }
    } else if (recovery->failure_count > recovery->max_retries) {
        recovery->state = RECOVERY_STATE_FAILED;
        
        if (g_recovery_manager.log_file) {
            fprintf(g_recovery_manager.log_file,
                    "[%llu] RECOVERY_FAILED: Transport %d exceeded max retries (%d)\n",
                    (unsigned long long)now, transport_type, recovery->max_retries);
            fflush(g_recovery_manager.log_file);
        }
    }
    
    pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
}

static void* transport_recovery_thread(void* arg) {
    transport_recovery_t* recovery = (transport_recovery_t*)arg;
    
    while (!recovery->shutdown_requested && 
           recovery->connection_lost && 
           recovery->failure_count <= recovery->max_retries) {
        
        int retry_interval = calculate_retry_interval(recovery);
        usleep(retry_interval * 1000);  // Convert to microseconds
        
        if (recovery->shutdown_requested) break;
        
        // Attempt to restart transport
        int result = transport_manager_restart_transport(recovery->transport_type);
        uint64_t now = get_timestamp_ms();
        
        if (result == 0) {
            // Recovery successful
            pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
            
            recovery->connection_lost = false;
            recovery->failure_count = 0;
            recovery->state = RECOVERY_STATE_SUCCESS;
            recovery->successful_recoveries++;
            recovery->total_recovery_time += now - recovery->recovery_start_time;
            g_recovery_manager.total_recoveries++;
            
            pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
            
            if (g_recovery_manager.log_file) {
                fprintf(g_recovery_manager.log_file,
                        "[%llu] RECOVERY_SUCCESS: Transport %d recovered after %llu ms\n",
                        (unsigned long long)now, recovery->transport_type,
                        (unsigned long long)(now - recovery->recovery_start_time));
                fflush(g_recovery_manager.log_file);
            }
            
            break;
        } else {
            // Recovery attempt failed
            pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
            recovery->failure_count++;
            pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
            
            if (g_recovery_manager.log_file) {
                fprintf(g_recovery_manager.log_file,
                        "[%llu] RECOVERY_ATTEMPT: Transport %d attempt %d failed (retry in %d ms)\n",
                        (unsigned long long)now, recovery->transport_type, 
                        recovery->failure_count, retry_interval);
                fflush(g_recovery_manager.log_file);
            }
        }
    }
    
    // Final state update
    pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
    
    if (recovery->connection_lost && recovery->failure_count > recovery->max_retries) {
        recovery->state = RECOVERY_STATE_FAILED;
        recovery->total_recovery_time += get_timestamp_ms() - recovery->recovery_start_time;
        
        if (g_recovery_manager.log_file) {
            fprintf(g_recovery_manager.log_file,
                    "[%llu] RECOVERY_EXHAUSTED: Transport %d failed after %d attempts\n",
                    (unsigned long long)get_timestamp_ms(), 
                    recovery->transport_type, recovery->max_retries);
            fflush(g_recovery_manager.log_file);
        }
    } else if (!recovery->connection_lost) {
        recovery->state = RECOVERY_STATE_IDLE;
    }
    
    pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
    
    return NULL;
}

bool transport_recovery_is_active(transport_type_t transport_type) {
    if (!g_recovery_manager.initialized || transport_type >= TRANSPORT_TYPE_COUNT) {
        return false;
    }
    
    pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
    bool active = (g_recovery_manager.transports[transport_type].state == RECOVERY_STATE_ACTIVE);
    pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
    
    return active;
}

int transport_recovery_get_stats(transport_type_t transport_type,
                                int* failure_count,
                                recovery_state_t* recovery_state,
                                uint64_t* last_failure_time) {
    if (!g_recovery_manager.initialized || transport_type >= TRANSPORT_TYPE_COUNT ||
        !failure_count || !recovery_state || !last_failure_time) {
        return -1;
    }
    
    pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
    
    transport_recovery_t* recovery = &g_recovery_manager.transports[transport_type];
    *failure_count = recovery->failure_count;
    *recovery_state = recovery->state;
    *last_failure_time = recovery->last_failure_time;
    
    pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
    
    return 0;
}

void transport_recovery_reset_state(transport_type_t transport_type) {
    if (!g_recovery_manager.initialized || transport_type >= TRANSPORT_TYPE_COUNT) {
        return;
    }
    
    pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
    
    transport_recovery_t* recovery = &g_recovery_manager.transports[transport_type];
    recovery->connection_lost = false;
    recovery->failure_count = 0;
    recovery->state = RECOVERY_STATE_IDLE;
    memset(recovery->last_error_message, 0, sizeof(recovery->last_error_message));
    
    pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
}

int transport_recovery_configure(transport_type_t transport_type,
                               int max_retries,
                               int base_interval_ms,
                               int max_interval_ms) {
    if (!g_recovery_manager.initialized || transport_type >= TRANSPORT_TYPE_COUNT ||
        max_retries < 0 || base_interval_ms < 0 || max_interval_ms < base_interval_ms) {
        return -1;
    }
    
    pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
    
    transport_recovery_t* recovery = &g_recovery_manager.transports[transport_type];
    recovery->max_retries = max_retries;
    recovery->retry_interval_ms = base_interval_ms;
    recovery->max_retry_interval_ms = max_interval_ms;
    
    pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
    
    return 0;
}

void transport_recovery_print_report(FILE* output_file) {
    if (!output_file) output_file = stderr;
    if (!g_recovery_manager.initialized) {
        fprintf(output_file, "Transport recovery system not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_recovery_manager.recovery_mutex);
    
    fprintf(output_file, "\n🔧 TRANSPORT RECOVERY REPORT\n");
    fprintf(output_file, "============================\n");
    fprintf(output_file, "System Status: %s\n", 
            g_recovery_manager.auto_recovery_enabled ? "Auto Recovery Enabled" : "Manual Only");
    fprintf(output_file, "Total Failures: %llu\n", 
            (unsigned long long)g_recovery_manager.total_failures);
    fprintf(output_file, "Total Recoveries: %llu\n", 
            (unsigned long long)g_recovery_manager.total_recoveries);
    
    if (g_recovery_manager.total_failures > 0) {
        double success_rate = (double)g_recovery_manager.total_recoveries / g_recovery_manager.total_failures * 100.0;
        fprintf(output_file, "Recovery Success Rate: %.1f%%\n", success_rate);
    }
    
    fprintf(output_file, "\nPer-Transport Status:\n");
    fprintf(output_file, "---------------------\n");
    
    for (int i = 0; i < TRANSPORT_TYPE_COUNT; i++) {
        transport_recovery_t* recovery = &g_recovery_manager.transports[i];
        
        fprintf(output_file, "%s: %s", 
                transport_type_to_string(i), 
                recovery_state_to_string(recovery->state));
        
        if (recovery->failure_count > 0) {
            fprintf(output_file, " (failures: %d, successes: %d)", 
                    recovery->failure_count, recovery->successful_recoveries);
        }
        
        if (recovery->state == RECOVERY_STATE_ACTIVE) {
            uint64_t recovery_time = get_timestamp_ms() - recovery->recovery_start_time;
            fprintf(output_file, " [Recovery time: %llu ms]", 
                    (unsigned long long)recovery_time);
        }
        
        if (strlen(recovery->last_error_message) > 0) {
            fprintf(output_file, " - Last error: %s", recovery->last_error_message);
        }
        
        fprintf(output_file, "\n");
    }
    
    fprintf(output_file, "============================\n\n");
    
    pthread_mutex_unlock(&g_recovery_manager.recovery_mutex);
}