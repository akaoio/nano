#pragma once

#include "../mcp_base/mcp_base.h"

// HTTP transport for MCP - communicates via HTTP POST requests

typedef struct {
    char* host;
    int port;
    char* path;
    bool initialized;
    bool running;
} http_transport_config_t;

// HTTP transport functions
int http_transport_init(void* config);
int http_transport_send(const mcp_message_t* message);
int http_transport_recv(mcp_message_t* message, int timeout_ms);
void http_transport_shutdown(void);

// Get HTTP transport interface
mcp_transport_t* http_transport_get_interface(void);
