#include "log_message.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void log_message(const char* format, ...) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] ", timestamp);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    fflush(stdout);
}