#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H

/**
 * Logs a formatted message with timestamp
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void log_message(const char* format, ...);

#endif