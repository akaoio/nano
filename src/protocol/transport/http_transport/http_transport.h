#pragma once

#include "../transport_base.h"
#include "../../protocol/mcp_adapter.h"

// HTTP Transport - Raw HTTP request/response data transmission only
// Uses MCP Adapter for all protocol logic
// Pure transport layer - no MCP awareness

typedef struct {
    char* host;
    int port;
    char* path;
    bool initialized;
    bool running;
    bool connected;
    
    // HTTP-specific settings
    int timeout_ms;
    bool keep_alive;
} http_transport_config_t;

// HTTP transport functions - Raw data transmission only
int http_transport_init(void* config);
int http_transport_send_raw(const char* data, size_t len);
int http_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);
void http_transport_shutdown(void);

// Connection management
int http_transport_connect(void);
int http_transport_disconnect(void);
bool http_transport_is_connected(void);

// HTTP-specific utilities
int http_transport_send_http_request(const char* method, const char* path, const char* body, char* response, size_t response_size);
int http_transport_parse_http_response(const char* response, char* body, size_t body_size);

// Get HTTP transport interface
transport_base_t* http_transport_get_interface(void);
