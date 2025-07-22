#define _DEFAULT_SOURCE
#include "logger.h"
#include "../common/time_utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

static logger_t g_logger = {0};

const char* logger_level_to_string(log_level_t level) {
    static const char* level_strings[] = {
        "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
    };
    
    if (level >= 0 && level <= LOG_LEVEL_FATAL) {
        return level_strings[level];
    }
    return "UNKNOWN";
}

static void logger_write(log_level_t level, const char* function, const char* file, int line, const char* format, va_list args) {
    if (!g_logger.initialized || level < g_logger.level) {
        return;
    }
    
    pthread_mutex_lock(&g_logger.log_mutex);
    
    // Get timestamp
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm* tm_info = localtime(&ts.tv_sec);
    
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char message[1024];
    vsnprintf(message, sizeof(message), format, args);
    
    char log_entry[2048];
    size_t entry_len = 0;
    
    if (g_logger.json_format) {
        // JSON format logging
        snprintf(log_entry, sizeof(log_entry),
                "{\"timestamp\":\"%s.%03ld\",\"level\":\"%s\",\"function\":\"%s\",\"file\":\"%s\",\"line\":%d,\"message\":\"%s\"}\n",
                timestamp, ts.tv_nsec / 1000000, logger_level_to_string(level), 
                function, file, line, message);
    } else {
        // Traditional format logging
        snprintf(log_entry, sizeof(log_entry),
                "[%s.%03ld] %s [%s:%d] %s: %s\n",
                timestamp, ts.tv_nsec / 1000000, logger_level_to_string(level), 
                file, line, function, message);
    }
    
    entry_len = strlen(log_entry);
    
    // Write to file
    if (g_logger.file) {
        fputs(log_entry, g_logger.file);
        fflush(g_logger.file);
        g_logger.current_file_size += entry_len;
        g_logger.log_entries_written++;
    }
    
    // Write to console
    if (g_logger.console_output) {
        if (level >= LOG_LEVEL_ERROR) {
            fprintf(stderr, "%s", log_entry);
        } else {
            printf("%s", log_entry);
        }
    }
    
    // Check for log rotation
    if (g_logger.file && g_logger.current_file_size > g_logger.max_file_size) {
        logger_rotate_file();
    }
    
    pthread_mutex_unlock(&g_logger.log_mutex);
}

int logger_init(const char* log_file_path, log_level_t level, bool json_format) {
    if (g_logger.initialized) {
        return 0; // Already initialized
    }
    
    if (!log_file_path) {
        return -1;
    }
    
    memset(&g_logger, 0, sizeof(logger_t));
    
    g_logger.level = level;
    g_logger.console_output = true;
    g_logger.json_format = json_format;
    g_logger.max_file_size = 10 * 1024 * 1024; // 10MB
    g_logger.max_backup_files = 5;
    
    strncpy(g_logger.log_file_path, log_file_path, sizeof(g_logger.log_file_path) - 1);
    
    // Open log file
    g_logger.file = fopen(log_file_path, "a");
    if (!g_logger.file) {
        fprintf(stderr, "Failed to open log file: %s (%s)\n", log_file_path, strerror(errno));
        return -1;
    }
    
    // Get current file size
    struct stat st;
    if (stat(log_file_path, &st) == 0) {
        g_logger.current_file_size = st.st_size;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&g_logger.log_mutex, NULL) != 0) {
        fclose(g_logger.file);
        return -1;
    }
    
    g_logger.initialized = true;
    
    // Log initialization
    logger_info("Logger initialized: level=%s, json=%s, file=%s, max_size=%zuMB",
                logger_level_to_string(level), json_format ? "true" : "false", 
                log_file_path, g_logger.max_file_size / (1024 * 1024));
    
    return 0;
}

void logger_shutdown(void) {
    if (!g_logger.initialized) {
        return;
    }
    
    logger_info("Logger shutting down (entries written: %ld)", g_logger.log_entries_written);
    
    pthread_mutex_lock(&g_logger.log_mutex);
    
    if (g_logger.file) {
        fclose(g_logger.file);
        g_logger.file = NULL;
    }
    
    pthread_mutex_unlock(&g_logger.log_mutex);
    
    pthread_mutex_destroy(&g_logger.log_mutex);
    g_logger.initialized = false;
}

bool logger_is_initialized(void) {
    return g_logger.initialized;
}

void logger_set_level(log_level_t level) {
    if (g_logger.initialized) {
        g_logger.level = level;
        logger_info("Log level changed to: %s", logger_level_to_string(level));
    }
}

log_level_t logger_get_level(void) {
    return g_logger.level;
}

void logger_set_console_output(bool enabled) {
    if (g_logger.initialized) {
        g_logger.console_output = enabled;
        logger_info("Console output %s", enabled ? "enabled" : "disabled");
    }
}

int logger_rotate_file(void) {
    if (!g_logger.initialized || !g_logger.file) {
        return -1;
    }
    
    // Close current file
    fclose(g_logger.file);
    g_logger.file = NULL;
    
    // Rotate backup files
    for (int i = g_logger.max_backup_files - 1; i >= 1; i--) {
        char old_name[512], new_name[512];
        
        if (i == 1) {
            snprintf(old_name, sizeof(old_name), "%s", g_logger.log_file_path);
        } else {
            snprintf(old_name, sizeof(old_name), "%s.%d", g_logger.log_file_path, i - 1);
        }
        
        snprintf(new_name, sizeof(new_name), "%s.%d", g_logger.log_file_path, i);
        
        // Rename file (ignore errors for non-existent files)
        rename(old_name, new_name);
    }
    
    // Open new log file
    g_logger.file = fopen(g_logger.log_file_path, "w");
    if (!g_logger.file) {
        fprintf(stderr, "Failed to create new log file: %s\n", g_logger.log_file_path);
        return -1;
    }
    
    g_logger.current_file_size = 0;
    
    logger_info("Log file rotated successfully");
    
    return 0;
}

void logger_log(log_level_t level, const char* function, const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logger_write(level, function, file, line, format, args);
    va_end(args);
}

int logger_get_stats(long* entries_written, size_t* current_file_size, size_t* max_file_size) {
    if (!g_logger.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_logger.log_mutex);
    
    if (entries_written) *entries_written = g_logger.log_entries_written;
    if (current_file_size) *current_file_size = g_logger.current_file_size;
    if (max_file_size) *max_file_size = g_logger.max_file_size;
    
    pthread_mutex_unlock(&g_logger.log_mutex);
    
    return 0;
}