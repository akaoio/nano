#ifndef SYSTEM_LOGGER_H
#define SYSTEM_LOGGER_H

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>

/**
 * @file logger.h
 * @brief Production Logging System
 * 
 * Provides comprehensive logging with multiple levels, JSON format support,
 * file rotation, and thread-safe operations.
 */

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} log_level_t;

typedef struct {
    log_level_t level;
    FILE* file;
    bool console_output;
    bool json_format;
    char log_file_path[256];
    pthread_mutex_t log_mutex;
    bool initialized;
    size_t max_file_size;
    int max_backup_files;
    size_t current_file_size;
    long log_entries_written;
} logger_t;

/**
 * @brief Initialize the logging system
 * @param log_file_path Path to log file
 * @param level Minimum log level to record
 * @param json_format Use JSON format for log entries
 * @return 0 on success, -1 on failure
 */
int logger_init(const char* log_file_path, log_level_t level, bool json_format);

/**
 * @brief Shutdown the logging system
 */
void logger_shutdown(void);

/**
 * @brief Check if logger is initialized
 * @return true if initialized, false otherwise
 */
bool logger_is_initialized(void);

/**
 * @brief Set log level dynamically
 * @param level New minimum log level
 */
void logger_set_level(log_level_t level);

/**
 * @brief Get current log level
 * @return Current log level
 */
log_level_t logger_get_level(void);

/**
 * @brief Enable or disable console output
 * @param enabled true to enable console output
 */
void logger_set_console_output(bool enabled);

/**
 * @brief Rotate log file manually
 * @return 0 on success, -1 on failure
 */
int logger_rotate_file(void);

/**
 * @brief Core logging function
 * @param level Log level
 * @param function Function name
 * @param file Source file name
 * @param line Line number
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void logger_log(log_level_t level, const char* function, const char* file, int line, const char* format, ...);

/**
 * @brief Get logging statistics
 * @param entries_written Output: number of log entries written
 * @param current_file_size Output: current log file size
 * @param max_file_size Output: maximum file size before rotation
 * @return 0 on success, -1 on failure
 */
int logger_get_stats(long* entries_written, size_t* current_file_size, size_t* max_file_size);

// Convenience macros for different log levels
#define logger_debug(fmt, ...) logger_log(LOG_LEVEL_DEBUG, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define logger_info(fmt, ...) logger_log(LOG_LEVEL_INFO, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define logger_warn(fmt, ...) logger_log(LOG_LEVEL_WARN, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define logger_error(fmt, ...) logger_log(LOG_LEVEL_ERROR, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define logger_fatal(fmt, ...) logger_log(LOG_LEVEL_FATAL, __FUNCTION__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// Get string representation of log level
const char* logger_level_to_string(log_level_t level);

#endif // SYSTEM_LOGGER_H