#ifndef NANO_TRANSPORT_MANAGER_H
#define NANO_TRANSPORT_MANAGER_H

#include "transport_base.h"
#include "../protocol/mcp_adapter.h"

// Transport Manager - Coordinates transport layer with MCP adapter
// Single integration point for all transports with centralized MCP protocol handling

typedef enum {
    TRANSPORT_MANAGER_OK = 0,
    TRANSPORT_MANAGER_ERROR = -1,
    TRANSPORT_MANAGER_INVALID_TRANSPORT = -2,
    TRANSPORT_MANAGER_NOT_CONNECTED = -3,
    TRANSPORT_MANAGER_PROTOCOL_ERROR = -4
} transport_manager_result_t;

typedef struct {
    transport_base_t* transport;
    mcp_adapter_t* mcp_adapter;
    bool initialized;
    bool connected;
    
    // Statistics
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t errors_count;
} transport_manager_t;

// Transport Manager API
int transport_manager_init(transport_manager_t* manager, transport_base_t* transport);
void transport_manager_shutdown(transport_manager_t* manager);

// Connection management
int transport_manager_connect(transport_manager_t* manager);
int transport_manager_disconnect(transport_manager_t* manager);
bool transport_manager_is_connected(transport_manager_t* manager);

// MCP Message handling (Transport -> MCP -> IO -> MCP -> Transport)
int transport_manager_send_mcp_request(transport_manager_t* manager, const mcp_request_t* request);
int transport_manager_send_mcp_response(transport_manager_t* manager, const mcp_response_t* response);
int transport_manager_recv_mcp_message(transport_manager_t* manager, char* raw_data, size_t data_size, int timeout_ms);

// Streaming support
int transport_manager_handle_stream_request(transport_manager_t* manager, const mcp_request_t* request, mcp_response_t* response);
int transport_manager_send_stream_chunk(transport_manager_t* manager, const mcp_stream_chunk_t* chunk);

// Utility functions
const char* transport_manager_result_to_string(transport_manager_result_t result);
void transport_manager_get_stats(transport_manager_t* manager, uint32_t* sent, uint32_t* received, uint32_t* errors);

#endif