#include "tcp_transport.h"
#include "../../../common/core.h"
#include "../../../common/transport_utils/transport_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static tcp_transport_config_t g_config = {0};

int tcp_transport_init(void* config) {
    if (!config) return -1;
    
    tcp_transport_config_t* cfg = (tcp_transport_config_t*)config;
    g_config = *cfg;
    
    // Create TCP socket
    g_config.socket_fd = create_tcp_socket();
    if (g_config.socket_fd < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    const char* host = g_config.is_server ? nullptr : g_config.host;
    
    if (setup_socket_address(&addr, host, g_config.port) != 0) {
        close_socket(g_config.socket_fd);
        return -1;
    }
    
    if (g_config.is_server) {
        // Server mode
        if (setup_server_socket(g_config.socket_fd, &addr, true) != 0) {
            close_socket(g_config.socket_fd);
            return -1;
        }
    } else {
        // Client mode
        if (connect_socket(g_config.socket_fd, &addr) != 0) {
            close_socket(g_config.socket_fd);
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
    
    // Check if socket is ready for reading
    int result = socket_select_read(g_config.socket_fd, timeout_ms);
    if (result <= 0) {
        return -1; // Timeout or error
    }
    
    // Read data
    char buffer[8192];
    ssize_t received = recv(g_config.socket_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (standard_buffer_processing(buffer, sizeof(buffer), received) != 0) {
        return -1;
    }
    
    // Parse JSON-RPC message
    return mcp_parse_json_rpc(buffer, message);
}

void tcp_transport_shutdown(void) {
    close_socket(g_config.socket_fd);
    g_config.socket_fd = -1;
    
    str_free(g_config.host);
    g_config.host = nullptr;
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
