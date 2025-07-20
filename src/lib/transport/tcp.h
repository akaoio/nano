#pragma once

#include "base.h"
#include "../protocol/adapter.h"
#include <netinet/in.h>

// TCP Transport - Raw TCP socket data transmission only
// Uses MCP Adapter for all protocol logic
// Pure transport layer - no MCP awareness

typedef struct {
    char* host;
    int port;
    int socket_fd;
    bool is_server;
    bool initialized;
    bool running;
    bool connected;
} tcp_transport_config_t;

// TCP transport functions - Raw data transmission only
int tcp_transport_init(void* config);
int tcp_transport_send_raw(const char* data, size_t len);
int tcp_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);
void tcp_transport_shutdown(void);

// Connection management
int tcp_transport_connect(void);
int tcp_transport_disconnect(void);
bool tcp_transport_is_connected(void);

// Get TCP transport interface
transport_base_t* tcp_transport_get_interface(void);
