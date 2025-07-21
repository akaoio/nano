/**
 * Process Manager
 * Handles PID file management and process detection
 */

// For usleep and popen/pclose
#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "process_manager.h"
#include "settings_global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

// Get PID file path from settings or use default
static const char* get_pid_file_path(void) {
    const char* pid_file = SETTING_SERVER(pid_file);
    return pid_file ? pid_file : "/tmp/mcp_server.pid";
}
#define MAX_PID_LEN 32

// Check if a process with given PID exists
static bool is_process_running(pid_t pid) {
    // Send signal 0 to check if process exists
    return (kill(pid, 0) == 0);
}

// Read PID from file
static pid_t read_pid_file(void) {
    FILE* f = fopen(get_pid_file_path(), "r");
    if (!f) {
        return -1;
    }
    
    char pid_str[MAX_PID_LEN];
    if (fgets(pid_str, sizeof(pid_str), f) == NULL) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    return (pid_t)atoi(pid_str);
}

// Write PID to file
static int write_pid_file(pid_t pid) {
    FILE* f = fopen(get_pid_file_path(), "w");
    if (!f) {
        fprintf(stderr, "Failed to create PID file: %s\n", strerror(errno));
        return -1;
    }
    
    fprintf(f, "%d\n", pid);
    fclose(f);
    return 0;
}

// Remove PID file
static void remove_pid_file(void) {
    unlink(get_pid_file_path());
}

// Check if port is in use
bool process_manager_is_port_in_use(int port) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "lsof -i :%d -t 2>/dev/null", port);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        return false;
    }
    
    char result[64];
    bool in_use = (fgets(result, sizeof(result), fp) != NULL);
    pclose(fp);
    
    return in_use;
}

// Find PIDs using specific port
int process_manager_get_pids_using_port(int port, pid_t* pids, int max_pids) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "lsof -i :%d -t 2>/dev/null", port);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        return 0;
    }
    
    int count = 0;
    char pid_str[32];
    while (fgets(pid_str, sizeof(pid_str), fp) != NULL && count < max_pids) {
        pids[count++] = (pid_t)atoi(pid_str);
    }
    
    pclose(fp);
    return count;
}

// Check if another instance is running
process_status_t process_manager_check_existing(void) {
    process_status_t status = {0};
    
    // Check PID file
    pid_t existing_pid = read_pid_file();
    if (existing_pid > 0) {
        if (is_process_running(existing_pid)) {
            status.is_running = true;
            status.pid = existing_pid;
            status.pid_file_exists = true;
            
            // Get process name
            char proc_path[256];
            snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", existing_pid);
            
            FILE* f = fopen(proc_path, "r");
            if (f) {
                fgets(status.process_name, sizeof(status.process_name), f);
                // Remove newline
                char* newline = strchr(status.process_name, '\n');
                if (newline) *newline = '\0';
                fclose(f);
            }
        } else {
            // Stale PID file
            status.pid_file_exists = true;
            status.is_stale = true;
            printf("⚠️  Found stale PID file (PID %d no longer running)\n", existing_pid);
            remove_pid_file();
        }
    }
    
    return status;
}

// Scan for processes using our ports
int process_manager_scan_ports(const process_port_scan_t* ports, int port_count, 
                               process_conflict_t* conflicts, int max_conflicts) {
    int conflict_count = 0;
    
    for (int i = 0; i < port_count && conflict_count < max_conflicts; i++) {
        if (!ports[i].enabled) continue;
        
        pid_t pids[10];
        int pid_count = process_manager_get_pids_using_port(ports[i].port, pids, 10);
        
        if (pid_count > 0) {
            for (int j = 0; j < pid_count && conflict_count < max_conflicts; j++) {
                conflicts[conflict_count].port = ports[i].port;
                conflicts[conflict_count].pid = pids[j];
                strncpy(conflicts[conflict_count].transport_name, ports[i].name, 
                        sizeof(conflicts[conflict_count].transport_name) - 1);
                
                // Try to get process name
                char proc_path[256];
                snprintf(proc_path, sizeof(proc_path), "/proc/%d/comm", pids[j]);
                
                FILE* f = fopen(proc_path, "r");
                if (f) {
                    fgets(conflicts[conflict_count].process_name, 
                          sizeof(conflicts[conflict_count].process_name), f);
                    // Remove newline
                    char* newline = strchr(conflicts[conflict_count].process_name, '\n');
                    if (newline) *newline = '\0';
                    fclose(f);
                } else {
                    snprintf(conflicts[conflict_count].process_name, 
                            sizeof(conflicts[conflict_count].process_name), 
                            "PID %d", pids[j]);
                }
                
                conflict_count++;
            }
        }
    }
    
    return conflict_count;
}

// Kill process gracefully, then forcefully if needed
int process_manager_kill_process(pid_t pid, bool force) {
    if (!is_process_running(pid)) {
        return 0; // Already dead
    }
    
    // First try SIGTERM
    printf("📤 Sending SIGTERM to process %d...\n", pid);
    if (kill(pid, SIGTERM) != 0) {
        fprintf(stderr, "Failed to send SIGTERM: %s\n", strerror(errno));
        return -1;
    }
    
    // Wait up to 3 seconds for graceful shutdown
    for (int i = 0; i < 30; i++) {
        usleep(100000); // 100ms
        if (!is_process_running(pid)) {
            printf("✅ Process %d terminated gracefully\n", pid);
            return 0;
        }
    }
    
    if (force) {
        // Force kill
        printf("⚠️  Process %d didn't terminate, sending SIGKILL...\n", pid);
        if (kill(pid, SIGKILL) != 0) {
            fprintf(stderr, "Failed to send SIGKILL: %s\n", strerror(errno));
            return -1;
        }
        
        // Wait a bit more
        usleep(500000); // 500ms
        
        if (!is_process_running(pid)) {
            printf("✅ Process %d killed forcefully\n", pid);
            return 0;
        } else {
            fprintf(stderr, "❌ Failed to kill process %d\n", pid);
            return -1;
        }
    } else {
        fprintf(stderr, "⚠️  Process %d still running (use --force to kill)\n", pid);
        return -1;
    }
}

// Initialize process management (create PID file)
int process_manager_init(void) {
    pid_t current_pid = getpid();
    
    if (write_pid_file(current_pid) != 0) {
        return -1;
    }
    
    printf("📝 Created PID file: %s (PID: %d)\n", get_pid_file_path(), current_pid);
    return 0;
}

// Cleanup process management (remove PID file)
void process_manager_cleanup(void) {
    pid_t current_pid = getpid();
    pid_t file_pid = read_pid_file();
    
    // Only remove if it's our PID
    if (file_pid == current_pid) {
        remove_pid_file();
        printf("🗑️  Removed PID file\n");
    }
}

// Kill conflicting processes
int process_manager_kill_conflicts(const process_conflict_t* conflicts, int conflict_count, bool force) {
    int killed = 0;
    
    for (int i = 0; i < conflict_count; i++) {
        printf("\n🎯 Attempting to kill %s (PID %d) using port %d...\n", 
               conflicts[i].process_name, conflicts[i].pid, conflicts[i].port);
               
        if (process_manager_kill_process(conflicts[i].pid, force) == 0) {
            killed++;
        }
    }
    
    return killed;
}