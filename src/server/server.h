#ifndef MCP_SERVER_H
#define MCP_SERVER_H

#include "core/core.h"
#include "protocol/mcp_adapter.h"
#include "transport/transport_manager.h"
#include "operations.h"
#include <stdbool.h>
#include <stdint.h>

// Unified MCP Server
// Combines IO operations, transport layer, and MCP protocol handling

typedef struct {
    bool initialized;
    bool running;
    
    // MCP Protocol Layer
    mcp_adapter_t* mcp_adapter;
    
    // Transport Management
    transport_manager_t* transport_managers;
    size_t transport_count;
    
    // Server Configuration
    char server_name[64];
    char version[16];
    uint16_t default_port;
    
    // Statistics
    uint64_t requests_processed;
    uint64_t responses_sent;
    uint64_t errors_handled;
    uint64_t uptime_seconds;
} mcp_server_t;

typedef struct {
    // Transport configurations
    bool enable_stdio;
    bool enable_tcp;
    bool enable_udp;
    bool enable_http;
    bool enable_websocket;
    
    // Server settings
    const char* server_name;
    uint16_t tcp_port;
    uint16_t udp_port;
    uint16_t http_port;
    uint16_t ws_port;
    const char* http_path;
    const char* ws_path;
    
    // Features
    bool enable_streaming;
    bool enable_logging;
    const char* log_file;
} mcp_server_config_t;

// Server lifecycle
int mcp_server_init(mcp_server_t* server, const mcp_server_config_t* config);
int mcp_server_start(mcp_server_t* server);
int mcp_server_stop(mcp_server_t* server);
void mcp_server_shutdown(mcp_server_t* server);

// Server operations
int mcp_server_process_request(mcp_server_t* server, const char* raw_request, char* response, size_t response_size);
int mcp_server_handle_streaming(mcp_server_t* server, const char* stream_id);

// Server management
int mcp_server_add_transport(mcp_server_t* server, transport_base_t* transport, void* config);
void mcp_server_get_stats(mcp_server_t* server, uint64_t* requests, uint64_t* responses, uint64_t* errors, uint64_t* uptime);
const char* mcp_server_get_status(mcp_server_t* server);

// Utility functions
int mcp_server_validate_config(const mcp_server_config_t* config);
void mcp_server_log(mcp_server_t* server, const char* level, const char* message);

#endif