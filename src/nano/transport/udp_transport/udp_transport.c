#include "udp_transport.h"
#include "../../../common/core.h"
#include "../../../common/transport_utils/transport_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static udp_transport_config_t g_config = {0};

int udp_transport_init(void* config) {
    if (!config) return -1;
    
    udp_transport_config_t* cfg = (udp_transport_config_t*)config;
    g_config = *cfg;
    
    // Create UDP socket
    g_config.socket_fd = create_udp_socket();
    if (g_config.socket_fd < 0) {
        return -1;
    }
    
    // Setup address
    if (setup_socket_address(&g_config.addr, g_config.host, g_config.port) != 0) {
        close_socket(g_config.socket_fd);
        return -1;
    }
    
    // Bind for receiving
    if (setup_server_socket(g_config.socket_fd, &g_config.addr, false) != 0) {
        close_socket(g_config.socket_fd);
        return -1;
    }
    
    g_config.initialized = true;
    g_config.running = true;
    return 0;
}

int udp_transport_send(const mcp_message_t* message) {
    if (!g_config.initialized || !message) return -1;
    
    char buffer[8192];
    if (mcp_format_json_rpc(message, buffer, sizeof(buffer)) != 0) {
        return -1;
    }
    
    ssize_t sent = sendto(g_config.socket_fd, buffer, strlen(buffer), 0,
                         (struct sockaddr*)&g_config.addr, sizeof(g_config.addr));
    return (sent > 0) ? 0 : -1;
}

int udp_transport_recv(mcp_message_t* message, int timeout_ms) {
    if (!g_config.initialized || !message) return -1;
    
    // Check if socket is ready for reading
    int result = socket_select_read(g_config.socket_fd, timeout_ms);
    if (result <= 0) {
        return -1; // Timeout or error
    }
    
    // Read data
    char buffer[8192];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ssize_t received = recvfrom(g_config.socket_fd, buffer, sizeof(buffer) - 1, 0,
                               (struct sockaddr*)&from_addr, &from_len);
    
    if (standard_buffer_processing(buffer, sizeof(buffer), received) != 0) {
        return -1;
    }
    
    // Parse JSON-RPC message
    return mcp_parse_json_rpc(buffer, message);
}

void udp_transport_shutdown(void) {
    close_socket(g_config.socket_fd);
    g_config.socket_fd = -1;
    
    str_free(g_config.host);
    g_config.host = nullptr;
    g_config.running = false;
    g_config.initialized = false;
}

static mcp_transport_t g_udp_transport = {
    .init = udp_transport_init,
    .send = udp_transport_send,
    .recv = udp_transport_recv,
    .shutdown = udp_transport_shutdown
};

mcp_transport_t* udp_transport_get_interface(void) {
    return &g_udp_transport;
}
