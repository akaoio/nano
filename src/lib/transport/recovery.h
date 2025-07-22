#ifndef TRANSPORT_RECOVERY_H
#define TRANSPORT_RECOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include "manager.h"

/**
 * @file recovery.h
 * @brief Transport Recovery and Fault Tolerance System
 * 
 * Provides automatic recovery mechanisms for transport layer failures,
 * including connection loss, timeouts, and resource conflicts.
 */

typedef enum {
    RECOVERY_STATE_IDLE,
    RECOVERY_STATE_ACTIVE,
    RECOVERY_STATE_FAILED,
    RECOVERY_STATE_SUCCESS
} recovery_state_t;

typedef enum {
    FAILURE_TYPE_CONNECTION_LOST,
    FAILURE_TYPE_TIMEOUT,
    FAILURE_TYPE_RESOURCE_BUSY,
    FAILURE_TYPE_PERMISSION_DENIED,
    FAILURE_TYPE_PORT_CONFLICT,
    FAILURE_TYPE_NETWORK_ERROR,
    FAILURE_TYPE_UNKNOWN
} failure_type_t;

typedef struct {
    transport_type_t transport_type;    // Type of transport
    failure_type_t failure_type;        // Type of failure
    bool connection_lost;               // Connection status
    uint64_t last_failure_time;         // Timestamp of last failure
    uint64_t first_failure_time;        // Timestamp of first failure in current sequence
    int failure_count;                  // Number of consecutive failures
    int max_retries;                    // Maximum retry attempts
    int retry_interval_ms;              // Base retry interval
    int backoff_multiplier;             // Exponential backoff multiplier
    int max_retry_interval_ms;          // Maximum retry interval
    pthread_t recovery_thread;          // Recovery thread
    recovery_state_t state;             // Current recovery state
    bool shutdown_requested;            // Shutdown flag
    char last_error_message[256];       // Last error message
    uint64_t recovery_start_time;       // When recovery started
    uint64_t total_recovery_time;       // Total time spent in recovery
    int successful_recoveries;          // Number of successful recoveries
} transport_recovery_t;

typedef struct {
    transport_recovery_t transports[TRANSPORT_TYPE_COUNT];
    pthread_mutex_t recovery_mutex;
    bool initialized;
    FILE* log_file;
    uint64_t total_failures;
    uint64_t total_recoveries;
    bool auto_recovery_enabled;
} recovery_manager_t;

/**
 * @brief Initialize the transport recovery system
 * @param auto_recovery Enable automatic recovery
 * @return 0 on success, -1 on failure
 */
int transport_recovery_init(bool auto_recovery);

/**
 * @brief Shutdown the transport recovery system
 */
void transport_recovery_shutdown(void);

/**
 * @brief Handle a transport failure and initiate recovery
 * @param transport_type Type of transport that failed
 * @param failure_type Type of failure that occurred
 * @param error_message Optional error message
 */
void transport_recovery_handle_failure(transport_type_t transport_type, 
                                     failure_type_t failure_type,
                                     const char* error_message);

/**
 * @brief Check if a transport is currently in recovery
 * @param transport_type Type of transport to check
 * @return true if in recovery, false otherwise
 */
bool transport_recovery_is_active(transport_type_t transport_type);

/**
 * @brief Get recovery statistics for a transport
 * @param transport_type Type of transport
 * @param failure_count Output: current failure count
 * @param recovery_state Output: current recovery state
 * @param last_failure_time Output: last failure timestamp
 * @return 0 on success, -1 on failure
 */
int transport_recovery_get_stats(transport_type_t transport_type,
                                int* failure_count,
                                recovery_state_t* recovery_state,
                                uint64_t* last_failure_time);

/**
 * @brief Force immediate recovery attempt for a transport
 * @param transport_type Type of transport
 * @return 0 on success, -1 on failure
 */
int transport_recovery_force_recovery(transport_type_t transport_type);

/**
 * @brief Reset recovery state for a transport (after successful operation)
 * @param transport_type Type of transport
 */
void transport_recovery_reset_state(transport_type_t transport_type);

/**
 * @brief Configure recovery parameters for a transport
 * @param transport_type Type of transport
 * @param max_retries Maximum retry attempts
 * @param base_interval_ms Base retry interval in milliseconds
 * @param max_interval_ms Maximum retry interval in milliseconds
 * @return 0 on success, -1 on failure
 */
int transport_recovery_configure(transport_type_t transport_type,
                               int max_retries,
                               int base_interval_ms,
                               int max_interval_ms);

/**
 * @brief Enable or disable automatic recovery for a transport
 * @param transport_type Type of transport
 * @param enabled true to enable, false to disable
 */
void transport_recovery_set_enabled(transport_type_t transport_type, bool enabled);

/**
 * @brief Get overall recovery system statistics
 * @param total_failures Output: total failures across all transports
 * @param total_recoveries Output: total successful recoveries
 * @param active_recoveries Output: currently active recovery attempts
 * @return 0 on success, -1 on failure
 */
int transport_recovery_get_system_stats(uint64_t* total_failures,
                                       uint64_t* total_recoveries,
                                       int* active_recoveries);

/**
 * @brief Print recovery report to file or stderr
 * @param output_file File to write to (NULL for stderr)
 */
void transport_recovery_print_report(FILE* output_file);

/**
 * @brief Test transport recovery system
 * @param transport_type Transport to test
 * @param failure_type Type of failure to simulate
 * @return 0 on success, -1 on failure
 */
int transport_recovery_test(transport_type_t transport_type, failure_type_t failure_type);

// Internal recovery thread function (implemented in .c file)
// static void* transport_recovery_thread(void* arg);

// Helper functions (implemented in .c file)
// static int calculate_retry_interval(transport_recovery_t* recovery);
// static const char* failure_type_to_string(failure_type_t type);
// static const char* recovery_state_to_string(recovery_state_t state);
// static bool is_recoverable_failure(failure_type_t failure_type);

#endif // TRANSPORT_RECOVERY_H