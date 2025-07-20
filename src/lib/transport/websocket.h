#pragma once

#include "base.h"
#include "../protocol/adapter.h"
#include "mcp/transport.h"

// WebSocket Transport - Raw WebSocket frame data transmission only
// Uses MCP Adapter for all protocol logic
// Pure transport layer - no MCP awareness

// WebSocket transport functions - Raw data transmission only
int ws_transport_init(void* config);
int ws_transport_send_raw(const char* data, size_t len);
int ws_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);
void ws_transport_shutdown(void);

// Connection management
int ws_transport_connect(void);
int ws_transport_disconnect(void);
bool ws_transport_is_connected(void);

// WebSocket-specific utilities
int ws_transport_send_ws_frame(const char* data, size_t len, bool is_text);
int ws_transport_recv_ws_frame(char* buffer, size_t buffer_size);
int ws_transport_perform_handshake(void);

// Get WebSocket transport interface
transport_base_t* ws_transport_get_interface(void);
