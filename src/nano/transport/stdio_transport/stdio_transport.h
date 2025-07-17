#pragma once

#include "../mcp_base/mcp_base.h"

// STDIO transport for MCP - communicates via stdin/stdout
// Used for local process communication

typedef struct {
    bool initialized;
    bool running;
} stdio_transport_config_t;

// STDIO transport functions
int stdio_transport_init(void* config);
int stdio_transport_send(const mcp_message_t* message);
int stdio_transport_recv(mcp_message_t* message, int timeout_ms);
void stdio_transport_shutdown(void);

// Get STDIO transport interface
mcp_transport_t* stdio_transport_get_interface(void);
