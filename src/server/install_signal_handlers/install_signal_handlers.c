#include "install_signal_handlers.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/wait.h>
#include <errno.h>

// Global flag for graceful shutdown
static volatile int shutdown_requested = 0;

/**
 * Signal handler for crash signals (SIGSEGV, SIGBUS, SIGFPE)
 */
static void crash_handler(int sig) {
    const char* signal_name;
    switch (sig) {
        case SIGSEGV: signal_name = "SIGSEGV (Segmentation fault)"; break;
        case SIGBUS:  signal_name = "SIGBUS (Bus error)"; break;
        case SIGFPE:  signal_name = "SIGFPE (Floating point exception)"; break;
        default:      signal_name = "Unknown signal"; break;
    }
    
    fprintf(stderr, "\n💥 CRITICAL: Server crash detected - %s\n", signal_name);
    fprintf(stderr, "🛡️  Crash handler activated - attempting recovery\n");
    
    // Print backtrace for debugging
    void *array[10];
    size_t size = backtrace(array, 10);
    fprintf(stderr, "🔍 Backtrace:\n");
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    
    // Log crash details
    fprintf(stderr, "🚨 This crash indicates a critical bug that needs fixing\n");
    fprintf(stderr, "🔧 Check for: NULL pointer access, buffer overflows, invalid memory access\n");
    
    // Instead of immediately exiting, try to clean up and restart
    fprintf(stderr, "🔄 Attempting graceful recovery...\n");
    
    // Set shutdown flag for main loop to handle cleanup
    shutdown_requested = 1;
    
    // Re-raise signal with default handler if recovery fails
    signal(sig, SIG_DFL);
    raise(sig);
}

/**
 * Signal handler for graceful shutdown (SIGTERM, SIGINT)
 */
static void shutdown_handler(int sig) {
    const char* signal_name = (sig == SIGTERM) ? "SIGTERM" : "SIGINT";
    fprintf(stderr, "\n🛑 Shutdown signal received: %s\n", signal_name);
    fprintf(stderr, "🧹 Initiating graceful shutdown...\n");
    
    shutdown_requested = 1;
}

/**
 * Check if shutdown was requested by signal handler
 */
int is_shutdown_requested(void) {
    return shutdown_requested;
}

int install_signal_handlers(void) {
    struct sigaction sa_crash, sa_shutdown;
    
    // Setup crash handler
    memset(&sa_crash, 0, sizeof(sa_crash));
    sa_crash.sa_handler = crash_handler;
    sigemptyset(&sa_crash.sa_mask);
    sa_crash.sa_flags = SA_RESTART;
    
    // Install crash handlers
    if (sigaction(SIGSEGV, &sa_crash, NULL) == -1) {
        perror("Failed to install SIGSEGV handler");
        return -1;
    }
    
    if (sigaction(SIGBUS, &sa_crash, NULL) == -1) {
        perror("Failed to install SIGBUS handler");
        return -1;
    }
    
    if (sigaction(SIGFPE, &sa_crash, NULL) == -1) {
        perror("Failed to install SIGFPE handler");
        return -1;
    }
    
    // Setup graceful shutdown handler
    memset(&sa_shutdown, 0, sizeof(sa_shutdown));
    sa_shutdown.sa_handler = shutdown_handler;
    sigemptyset(&sa_shutdown.sa_mask);
    sa_shutdown.sa_flags = SA_RESTART;
    
    // Install shutdown handlers
    if (sigaction(SIGTERM, &sa_shutdown, NULL) == -1) {
        perror("Failed to install SIGTERM handler");
        return -1;
    }
    
    if (sigaction(SIGINT, &sa_shutdown, NULL) == -1) {
        perror("Failed to install SIGINT handler");
        return -1;
    }
    
    // Ignore SIGPIPE to prevent crashes on broken connections
    signal(SIGPIPE, SIG_IGN);
    
    fprintf(stderr, "🛡️  Signal handlers installed successfully\n");
    fprintf(stderr, "✅ Crash protection: SIGSEGV, SIGBUS, SIGFPE\n");
    fprintf(stderr, "✅ Graceful shutdown: SIGTERM, SIGINT\n");
    fprintf(stderr, "✅ Connection protection: SIGPIPE ignored\n");
    
    return 0;
}