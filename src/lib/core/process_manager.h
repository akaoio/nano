#pragma once

/**
 * Process Manager
 * Handles PID file management, port scanning, and process termination
 */

#include <stdbool.h>
#include <sys/types.h>

// Process status information
typedef struct {
    bool is_running;
    bool pid_file_exists;
    bool is_stale;
    pid_t pid;
    char process_name[256];
} process_status_t;

// Port scan configuration
typedef struct {
    int port;
    const char* name;
    bool enabled;
} process_port_scan_t;

// Port conflict information
typedef struct {
    int port;
    pid_t pid;
    char transport_name[64];
    char process_name[256];
} process_conflict_t;

// Check if another instance is running
process_status_t process_manager_check_existing(void);

// Check if a specific port is in use
bool process_manager_is_port_in_use(int port);

// Get PIDs using a specific port
int process_manager_get_pids_using_port(int port, pid_t* pids, int max_pids);

// Scan for port conflicts
int process_manager_scan_ports(const process_port_scan_t* ports, int port_count, 
                              process_conflict_t* conflicts, int max_conflicts);

// Kill a process (gracefully, then forcefully if force=true)
int process_manager_kill_process(pid_t pid, bool force);

// Kill conflicting processes
int process_manager_kill_conflicts(const process_conflict_t* conflicts, int conflict_count, bool force);

// Initialize process management (create PID file)
int process_manager_init(void);

// Cleanup process management (remove PID file)
void process_manager_cleanup(void);