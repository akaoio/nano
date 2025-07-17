#include "ws_transport.h"
#include "../../../common/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static ws_transport_config_t g_config = {0};

int ws_transport_init(void* config) {
    if (!config) return -1;
    
    ws_transport_config_t* cfg = (ws_transport_config_t*)config;
    g_config.host = str_copy(cfg->host);
    g_config.port = cfg->port;
    g_config.path = str_copy(cfg->path ? cfg->path : "/");
    g_config.socket_fd = -1;
    g_config.initialized = true;
    g_config.running = true;
    g_config.connected = false;
    
    // For simplicity, we'll implement a basic WebSocket-like protocol
    // Real implementation would need full WebSocket handshake
    
    return 0;
}

int ws_transport_send(const mcp_message_t* message) {
    if (!g_config.initialized || !message) return -1;
    
    // Connect if not connected
    if (!g_config.connected) {
        g_config.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (g_config.socket_fd < 0) return -1;
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(g_config.port);
        inet_pton(AF_INET, g_config.host, &addr.sin_addr);
        
        if (connect(g_config.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(g_config.socket_fd);
            g_config.socket_fd = -1;
            return -1;
        }
        
        g_config.connected = true;
    }
    
    // Format JSON-RPC message
    char buffer[8192];
    if (mcp_format_json_rpc(message, buffer, sizeof(buffer)) != 0) {
        return -1;
    }
    
    // Simple text frame (not full WebSocket protocol)
    char frame[8194]; // Slightly larger to accommodate newline and null terminator
    snprintf(frame, sizeof(frame), "%s\n", buffer);
    
    ssize_t sent = send(g_config.socket_fd, frame, strlen(frame), 0);
    return (sent > 0) ? 0 : -1;
}

int ws_transport_recv(mcp_message_t* message, int timeout_ms) {
    if (!g_config.initialized || !message || !g_config.connected) return -1;
    
    // Use select for timeout
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(g_config.socket_fd, &readfds);
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(g_config.socket_fd + 1, &readfds, NULL, NULL, &timeout);
    if (result <= 0) {
        return -1; // Timeout or error
    }
    
    // Read data
    char buffer[8192];
    ssize_t received = recv(g_config.socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        g_config.connected = false;
        return -1;
    }
    
    buffer[received] = '\0';
    
    // Remove trailing newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    // Parse JSON-RPC message
    return mcp_parse_json_rpc(buffer, message);
}

void ws_transport_shutdown(void) {
    if (g_config.socket_fd >= 0) {
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
    }
    
    str_free(g_config.host);
    str_free(g_config.path);
    g_config.host = NULL;
    g_config.path = NULL;
    g_config.connected = false;
    g_config.running = false;
    g_config.initialized = false;
}

static mcp_transport_t g_ws_transport = {
    .init = ws_transport_init,
    .send = ws_transport_send,
    .recv = ws_transport_recv,
    .shutdown = ws_transport_shutdown
};

mcp_transport_t* ws_transport_get_interface(void) {
    return &g_ws_transport;
}
