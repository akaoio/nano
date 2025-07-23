#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H

#include <syslog.h>

/**
 * Log levels for filtering output (mapped to syslog levels)
 */
typedef enum {
    LOG_LEVEL_DEBUG = LOG_DEBUG,    // 7 - Debug messages
    LOG_LEVEL_INFO = LOG_INFO,      // 6 - Informational messages  
    LOG_LEVEL_WARN = LOG_WARNING,   // 4 - Warning conditions
    LOG_LEVEL_ERROR = LOG_ERR       // 3 - Error conditions
} log_level_t;

/**
 * Initializes the logging system with syslog
 * @param ident Program identifier for syslog
 */
void init_logging(const char* ident);

/**
 * Logs a formatted message via syslog
 * @param level Log level (DEBUG, INFO, WARN, ERROR)
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void log_message(log_level_t level, const char* format, ...);

/**
 * Sets the minimum log level to display
 * @param level Minimum level to log
 */
void set_log_level(log_level_t level);

/**
 * Closes the logging system
 */
void close_logging(void);

/**
 * Convenience macros for different log levels
 * DEBUG messages are conditionally compiled out in production builds
 */
#ifdef DEBUG
#define LOG_DEBUG_MSG(fmt, ...) log_message(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG_MSG(fmt, ...) do { /* Debug logging disabled in production */ } while(0)
#endif

#define LOG_INFO_MSG(fmt, ...) log_message(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN_MSG(fmt, ...) log_message(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR_MSG(fmt, ...) log_message(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

/**
 * Emergency logging function for bootstrap errors before syslog is initialized
 * Only for critical failures during initialization
 */
void emergency_log(const char* message);

#endif