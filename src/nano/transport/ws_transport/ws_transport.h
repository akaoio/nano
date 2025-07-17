#pragma once

#include "../mcp_base/mcp_base.h"

// WebSocket transport for MCP - communicates via WebSocket connections

typedef struct {
    char* host;
    int port;
    char* path;
    int socket_fd;
    bool initialized;
    bool running;
    bool connected;
} ws_transport_config_t;

// WebSocket transport functions
int ws_transport_init(void* config);
int ws_transport_send(const mcp_message_t* message);
int ws_transport_recv(mcp_message_t* message, int timeout_ms);
void ws_transport_shutdown(void);

// Get WebSocket transport interface
mcp_transport_t* ws_transport_get_interface(void);
