#ifndef MCP_SERVER_INTERNAL_H
#define MCP_SERVER_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>
#include "mcp/protocol.h"
#include "mcp/transport.h"
#include "mcp/server.h"
#include "operations.h"
#include "../protocol/adapter.h"
#include "../transport/manager.h"
#include "npu_queue.h"
#include "async_response.h"

// Unified MCP Server
// Combines IO operations, transport layer, and MCP protocol handling

// Forward declarations moved to includes

// Use the internal structure for implementation
// #define mcp_server_t mcp_server_internal_t

// Internal server structure for implementation use
typedef struct mcp_server_internal {
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
    
    // NPU Queue System
    npu_queue_t* npu_queue;
    async_response_registry_t* response_registry;
    
    // Current context for NPU operations
    int current_transport_index;
    void* current_connection;
    
    // Statistics
    uint64_t requests_processed;
    uint64_t responses_sent;
    uint64_t errors_handled;
    uint64_t uptime_seconds;
    uint64_t npu_operations_queued;
    uint64_t instant_operations_processed;
} mcp_server_internal_t;

// Internal server lifecycle (different names to avoid conflicts)
int mcp_server_internal_init(mcp_server_internal_t* server, const mcp_server_config_t* config);
int mcp_server_internal_start(mcp_server_internal_t* server);
int mcp_server_internal_run_event_loop(mcp_server_internal_t* server);
int mcp_server_internal_stop(mcp_server_internal_t* server);
void mcp_server_internal_shutdown(mcp_server_internal_t* server);

// Internal server operations
int mcp_server_internal_process_request(mcp_server_internal_t* server, const char* raw_request, char* response, size_t response_size);
int mcp_server_internal_handle_streaming(mcp_server_internal_t* server, const char* request_id);

// Internal server management
int mcp_server_internal_add_transport(mcp_server_internal_t* server, transport_base_t* transport, void* config);
void mcp_server_internal_get_stats(mcp_server_internal_t* server, uint64_t* requests, uint64_t* responses, uint64_t* errors, uint64_t* uptime);
const char* mcp_server_internal_get_status(mcp_server_internal_t* server);

// Internal utility functions
int mcp_server_internal_validate_config(const mcp_server_config_t* config);
void mcp_server_internal_log(mcp_server_internal_t* server, const char* level, const char* message);

#endif // MCP_SERVER_INTERNAL_H