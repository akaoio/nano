#pragma once

#include "../transport_base.h"
#include "../../protocol/mcp_adapter.h"
#include <netinet/in.h>

// UDP Transport - Raw UDP socket data transmission only
// Uses MCP Adapter for all protocol logic
// Pure transport layer - no MCP awareness

typedef struct {
    char* host;
    int port;
    int socket_fd;
    struct sockaddr_in addr;
    bool initialized;
    bool running;
    bool connected;
    
    // UDP-specific reliability features (transport-level only)
    bool enable_retry;
    int max_retries;
    int retry_timeout_ms;
} udp_transport_config_t;

// UDP transport functions - Raw data transmission only
int udp_transport_init(void* config);
int udp_transport_send_raw(const char* data, size_t len);
int udp_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);
void udp_transport_shutdown(void);

// Connection management
int udp_transport_connect(void);
int udp_transport_disconnect(void);
bool udp_transport_is_connected(void);

// UDP reliability features (transport-level only)
int udp_transport_send_with_retry(const char* data, size_t len);

// Get UDP transport interface
transport_base_t* udp_transport_get_interface(void);
