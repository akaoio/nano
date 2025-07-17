#include "tcp_transport.h"
#include "../../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static tcp_transport_config_t g_config = {0};

int tcp_transport_init(void* config) {
    if (!config) return -1;
    
    tcp_transport_config_t* cfg = (tcp_transport_config_t*)config;
    g_config = *cfg;
    
    // Create socket
    g_config.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_config.socket_fd < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_config.port);
    
    if (g_config.is_server) {
        // Server mode
        addr.sin_addr.s_addr = INADDR_ANY;
        
        int opt = 1;
        setsockopt(g_config.socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        if (bind(g_config.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(g_config.socket_fd);
            return -1;
        }
        
        if (listen(g_config.socket_fd, 1) < 0) {
            close(g_config.socket_fd);
            return -1;
        }
    } else {
        // Client mode
        if (inet_pton(AF_INET, g_config.host, &addr.sin_addr) <= 0) {
            close(g_config.socket_fd);
            return -1;
        }
        
        if (connect(g_config.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(g_config.socket_fd);
            return -1;
        }
    }
    
    g_config.initialized = true;
    g_config.running = true;
    return 0;
}

int tcp_transport_send(const mcp_message_t* message) {
    if (!g_config.initialized || !message) return -1;
    
    char buffer[8192];
    if (mcp_format_json_rpc(message, buffer, sizeof(buffer)) != 0) {
        return -1;
    }
    
    // Add newline delimiter
    strcat(buffer, "\n");
    
    ssize_t sent = send(g_config.socket_fd, buffer, strlen(buffer), 0);
    return (sent > 0) ? 0 : -1;
}

int tcp_transport_recv(mcp_message_t* message, int timeout_ms) {
    if (!g_config.initialized || !message) return -1;
    
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

void tcp_transport_shutdown(void) {
    if (g_config.socket_fd >= 0) {
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
    }
    
    str_free(g_config.host);
    g_config.host = NULL;
    g_config.running = false;
    g_config.initialized = false;
}

static mcp_transport_t g_tcp_transport = {
    .init = tcp_transport_init,
    .send = tcp_transport_send,
    .recv = tcp_transport_recv,
    .shutdown = tcp_transport_shutdown
};

mcp_transport_t* tcp_transport_get_interface(void) {
    return &g_tcp_transport;
}
