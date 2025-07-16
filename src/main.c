#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "io/io.h"

static volatile int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("🚀 Nano + IO - RKLLM Gateway\n");
    printf("============================\n");
    
    if (io_init() != 0) {
        fprintf(stderr, "Failed to initialize IO layer\n");
        return 1;
    }
    
    printf("✅ IO layer initialized successfully\n");
    printf("📡 Ready to accept requests...\n");
    printf("Press Ctrl+C to exit\n\n");
    
    while (running) {
        char response[8192];
        if (io_pop_response(response, sizeof(response)) == 0) {
            printf("📤 Response: %s\n", response);
        }
        
        usleep(10000); // 10ms
    }
    
    printf("\n🛑 Shutting down...\n");
    io_shutdown();
    printf("✅ Shutdown complete\n");
    
    return 0;
}
