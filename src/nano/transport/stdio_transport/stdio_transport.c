#include "stdio_transport.h"
#include "../../../common/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

static stdio_transport_config_t g_config = {0};

int stdio_transport_init(void* config) {
    (void)config;
    g_config.initialized = true;
    g_config.running = true;
    return 0;
}

int stdio_transport_send(const mcp_message_t* message) {
    if (!g_config.initialized || !message) return -1;
    
    char buffer[8192];
    if (mcp_format_json_rpc(message, buffer, sizeof(buffer)) != 0) {
        return -1;
    }
    
    // Send via stdout
    printf("%s\n", buffer);
    fflush(stdout);
    
    return 0;
}

int stdio_transport_recv(mcp_message_t* message, int timeout_ms) {
    if (!g_config.initialized || !message) return -1;
    
    // Use select to wait for input with timeout
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout);
    if (result <= 0) {
        return -1; // Timeout or error
    }
    
    // Read line from stdin
    char buffer[8192];
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return -1;
    }
    
    // Process buffer (remove trailing newline)
    process_received_buffer(buffer, strlen(buffer));
    
    // Parse JSON-RPC message
    return mcp_parse_json_rpc(buffer, message);
}

void stdio_transport_shutdown(void) {
    g_config.running = false;
    g_config.initialized = false;
}

static mcp_transport_t g_stdio_transport = {
    .init = stdio_transport_init,
    .send = stdio_transport_send,
    .recv = stdio_transport_recv,
    .shutdown = stdio_transport_shutdown
};

mcp_transport_t* stdio_transport_get_interface(void) {
    return &g_stdio_transport;
}
