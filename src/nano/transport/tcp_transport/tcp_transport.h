#pragma once

#include "../mcp_base/mcp_base.h"
#include <netinet/in.h>

// TCP transport for MCP - communicates via TCP sockets

typedef struct {
    char* host;
    int port;
    int socket_fd;
    bool is_server;
    bool initialized;
    bool running;
} tcp_transport_config_t;

// TCP transport functions
int tcp_transport_init(void* config);
int tcp_transport_send(const mcp_message_t* message);
int tcp_transport_recv(mcp_message_t* message, int timeout_ms);
void tcp_transport_shutdown(void);

// Get TCP transport interface
mcp_transport_t* tcp_transport_get_interface(void);
