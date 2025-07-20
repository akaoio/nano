#pragma once

#include "../transport_base.h"
#include "../../protocol/mcp_adapter.h"

// STDIO Transport - Raw stdin/stdout data transmission only
// Uses MCP Adapter for all protocol logic
// Pure transport layer - no MCP awareness

typedef struct {
    bool initialized;
    bool running;
    
    // STDIO specific settings
    bool log_to_stderr;     // Allow logging to stderr
    bool line_buffered;     // Line buffering for real-time communication
} stdio_transport_config_t;

// STDIO transport functions - Raw data transmission only
int stdio_transport_init(void* config);
int stdio_transport_send_raw(const char* data, size_t len);
int stdio_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);
void stdio_transport_shutdown(void);

// Connection management
int stdio_transport_connect(void);
int stdio_transport_disconnect(void);
bool stdio_transport_is_connected(void);

// Transport utilities
void stdio_transport_log_to_stderr(const char* message);

// Get STDIO transport interface
transport_base_t* stdio_transport_get_interface(void);
