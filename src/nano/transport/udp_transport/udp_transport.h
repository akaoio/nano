#pragma once

#include "../mcp_base/mcp_base.h"
#include <netinet/in.h>

// UDP transport for MCP - communicates via UDP sockets

typedef struct {
    char* host;
    int port;
    int socket_fd;
    struct sockaddr_in addr;
    bool initialized;
    bool running;
} udp_transport_config_t;

// UDP transport functions
int udp_transport_init(void* config);
int udp_transport_send(const mcp_message_t* message);
int udp_transport_recv(mcp_message_t* message, int timeout_ms);
void udp_transport_shutdown(void);

// Get UDP transport interface
mcp_transport_t* udp_transport_get_interface(void);
