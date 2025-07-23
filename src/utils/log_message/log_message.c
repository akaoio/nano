#include "log_message.h"
#include "../constants/constants.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

static log_level_t current_log_level = LOG_LEVEL_INFO;
static int syslog_initialized = 0;

void init_logging(const char* ident) {
    if (!syslog_initialized) {
        openlog(ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);
        syslog_initialized = 1;
    }
}

void set_log_level(log_level_t level) {
    current_log_level = level;
    
    // Set syslog mask to filter messages below current level
    if (syslog_initialized) {
        int mask = 0;
        for (int i = 0; i <= (int)level; i++) {
            mask |= LOG_MASK(i);
        }
        setlogmask(mask);
    }
}

void log_message(log_level_t level, const char* format, ...) {
    // Initialize syslog if not already done
    if (!syslog_initialized) {
        init_logging("rkllm-server");
    }
    
    // Check level filtering
    if (level < current_log_level) {
        return;
    }
    
    // Format message
    va_list args;
    va_start(args, format);
    
    // Use syslog's vsyslog function for formatted output
    vsyslog(level, format, args);
    
    va_end(args);
}

void close_logging(void) {
    if (syslog_initialized) {
        closelog();
        syslog_initialized = 0;
    }
}

void emergency_log(const char* message) {
    // Emergency logging to stderr for bootstrap failures
    // Only used when syslog cannot be initialized
    write(STDERR_FILENO, "[EMERGENCY] ", 12);
    write(STDERR_FILENO, message, strlen(message));
    write(STDERR_FILENO, "\n", 1);
}