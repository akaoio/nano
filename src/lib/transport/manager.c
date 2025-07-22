#include "manager.h"
#include "common/types.h"
#include "../core/settings_global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global transport manager instance for streaming
static transport_manager_t* g_global_transport_manager = NULL;

int transport_manager_init(transport_manager_t* manager, transport_base_t* transport) {
    if (!manager || !transport) return TRANSPORT_MANAGER_ERROR;
    
    memset(manager, 0, sizeof(transport_manager_t));
    manager->transport = transport;
    
    // Get buffer size from settings or use default
    size_t buffer_size = SETTING_BUFFER(transport_buffer_size);
    if (buffer_size == 0) buffer_size = 8192;  // fallback
    
    // Allocate dynamic buffers
    manager->request_buffer = malloc(buffer_size);
    manager->response_buffer = malloc(buffer_size);
    if (!manager->request_buffer || !manager->response_buffer) {
        free(manager->request_buffer);
        free(manager->response_buffer);
        return TRANSPORT_MANAGER_ERROR;
    }
    manager->buffer_size = buffer_size;
    
    // Initialize MCP adapter (use global instance)
    manager->mcp_adapter = &g_mcp_adapter;
    if (!manager->mcp_adapter->initialized) {
        if (mcp_adapter_init(manager->mcp_adapter) != MCP_ADAPTER_OK) {
            return TRANSPORT_MANAGER_ERROR;
        }
    }
    
    // Don't initialize transport here - let the caller do it with proper config
    
    manager->initialized = true;
    return TRANSPORT_MANAGER_OK;
}

void transport_manager_shutdown(transport_manager_t* manager) {
    if (!manager || !manager->initialized) return;
    
    // Disconnect transport
    if (manager->connected && manager->transport->disconnect) {
        manager->transport->disconnect(manager->transport);
    }
    
    // Shutdown transport
    if (manager->transport->shutdown) {
        manager->transport->shutdown(manager->transport);
    }
    
    // Free dynamic buffers
    free(manager->request_buffer);
    free(manager->response_buffer);
    manager->request_buffer = NULL;
    manager->response_buffer = NULL;
    manager->buffer_size = 0;
    
    // Note: Don't shutdown MCP adapter as it's global and may be used by other managers
    
    manager->connected = false;
    manager->initialized = false;
}

int transport_manager_connect(transport_manager_t* manager) {
    if (!manager || !manager->initialized || !manager->transport) {
        return TRANSPORT_MANAGER_INVALID_TRANSPORT;
    }
    
    if (manager->transport->connect) {
        int result = manager->transport->connect(manager->transport);
        if (result == 0) {
            manager->connected = true;
            return TRANSPORT_MANAGER_OK;
        }
    }
    
    return TRANSPORT_MANAGER_ERROR;
}

int transport_manager_disconnect(transport_manager_t* manager) {
    if (!manager || !manager->initialized || !manager->transport) {
        return TRANSPORT_MANAGER_INVALID_TRANSPORT;
    }
    
    if (manager->transport->disconnect) {
        manager->transport->disconnect(manager->transport);
    }
    
    manager->connected = false;
    return TRANSPORT_MANAGER_OK;
}

bool transport_manager_is_connected(transport_manager_t* manager) {
    if (!manager || !manager->initialized || !manager->transport) {
        return false;
    }
    
    if (manager->transport->is_connected) {
        bool transport_connected = (manager->transport->is_connected(manager->transport) == 1);
        manager->connected = transport_connected;
        return transport_connected;
    }
    
    return manager->connected;
}

int transport_manager_send_mcp_request(transport_manager_t* manager, const mcp_request_t* request) {
    if (!manager || !manager->initialized || !request) {
        return TRANSPORT_MANAGER_ERROR;
    }
    
    if (!transport_manager_is_connected(manager)) {
        return TRANSPORT_MANAGER_NOT_CONNECTED;
    }
    
    // Use dynamic buffer for formatting (use response buffer instead)
    char* formatted_data = manager->response_buffer;
    mcp_response_t temp_response;
    memset(&temp_response, 0, sizeof(temp_response));
    
    // Convert request to response format for formatting
    strncpy(temp_response.request_id, request->request_id, sizeof(temp_response.request_id) - 1);
    temp_response.is_success = true;
    strncpy(temp_response.result, request->params, sizeof(temp_response.result) - 1);
    
    // Format as JSON-RPC request (reverse engineer from response format)
    char* request_json = manager->response_buffer;
    snprintf(request_json, manager->buffer_size, 
        "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s,\"id\":%s}",
        request->method, request->params, request->request_id);
    
    // Add newline for MCP compliance
    size_t len = strlen(request_json);
    if (len < manager->buffer_size - 2) {
        request_json[len] = '\n';
        request_json[len + 1] = '\0';
        len++;
    }
    
    // Send via transport
    int result = manager->transport->send(manager->transport, request_json, len);
    if (result == 0) {
        manager->messages_sent++;
        return TRANSPORT_MANAGER_OK;
    } else {
        manager->errors_count++;
        return TRANSPORT_MANAGER_ERROR;
    }
}

int transport_manager_send_mcp_response(transport_manager_t* manager, const mcp_response_t* response) {
    if (!manager || !manager->initialized || !response) {
        return TRANSPORT_MANAGER_ERROR;
    }
    
    if (!transport_manager_is_connected(manager)) {
        return TRANSPORT_MANAGER_NOT_CONNECTED;
    }
    
    // Use dynamic buffer for formatting response
    char* formatted_data = manager->request_buffer;
    if (mcp_adapter_format_response(response, formatted_data, manager->buffer_size) != MCP_ADAPTER_OK) {
        manager->errors_count++;
        return TRANSPORT_MANAGER_PROTOCOL_ERROR;
    }
    
    // Add newline for MCP compliance
    size_t len = strlen(formatted_data);
    if (len < manager->buffer_size - 2) {
        formatted_data[len] = '\n';
        formatted_data[len + 1] = '\0';
        len++;
    }
    
    // Send via transport
    int result = manager->transport->send(manager->transport, formatted_data, len);
    if (result == 0) {
        manager->messages_sent++;
        return TRANSPORT_MANAGER_OK;
    } else {
        manager->errors_count++;
        return TRANSPORT_MANAGER_ERROR;
    }
}

int transport_manager_recv_mcp_message(transport_manager_t* manager, char* raw_data, size_t data_size, int timeout_ms) {
    if (!manager || !manager->initialized || !raw_data || data_size == 0) {
        return TRANSPORT_MANAGER_ERROR;
    }
    
    if (!transport_manager_is_connected(manager)) {
        return TRANSPORT_MANAGER_NOT_CONNECTED;
    }
    
    // Receive raw data from transport
    int result = manager->transport->recv(manager->transport, raw_data, data_size, timeout_ms);
    if (result == 0) {
        manager->messages_received++;
        
        // Remove trailing newline if present (MCP compliance)
        size_t len = strlen(raw_data);
        if (len > 0 && raw_data[len - 1] == '\n') {
            raw_data[len - 1] = '\0';
        }
        
        return TRANSPORT_MANAGER_OK;
    } else {
        manager->errors_count++;
        return TRANSPORT_MANAGER_ERROR;
    }
}

int transport_manager_handle_stream_request(transport_manager_t* manager, const mcp_request_t* request, mcp_response_t* response) {
    if (!manager || !manager->initialized || !request || !response) {
        return TRANSPORT_MANAGER_ERROR;
    }
    
    // Use MCP adapter to handle streaming
    int result = mcp_adapter_handle_stream_request(request, response);
    if (result == MCP_ADAPTER_OK) {
        return TRANSPORT_MANAGER_OK;
    } else {
        manager->errors_count++;
        return TRANSPORT_MANAGER_PROTOCOL_ERROR;
    }
}

int transport_manager_send_stream_chunk(transport_manager_t* manager, const mcp_stream_chunk_t* chunk) {
    if (!manager || !manager->initialized || !chunk) {
        return TRANSPORT_MANAGER_ERROR;
    }
    
    if (!transport_manager_is_connected(manager)) {
        return TRANSPORT_MANAGER_NOT_CONNECTED;
    }
    
    // Call transport-specific streaming method if available
    if (manager->transport->send_stream_chunk) {
        int result = manager->transport->send_stream_chunk(manager->transport, chunk);
        if (result == 0) {
            manager->messages_sent++;
            return TRANSPORT_MANAGER_OK;
        } else {
            manager->errors_count++;
            return TRANSPORT_MANAGER_ERROR;
        }
    }
    
    // Fallback to regular send method
    char* formatted_chunk = manager->request_buffer;
    if (mcp_adapter_format_stream_chunk(chunk, formatted_chunk, manager->buffer_size) != MCP_ADAPTER_OK) {
        manager->errors_count++;
        return TRANSPORT_MANAGER_PROTOCOL_ERROR;
    }
    
    // Add newline for MCP compliance
    size_t len = strlen(formatted_chunk);
    if (len < manager->buffer_size - 2) {
        formatted_chunk[len] = '\n';
        formatted_chunk[len + 1] = '\0';
        len++;
    }
    
    // Send via transport
    int result = manager->transport->send(manager->transport, formatted_chunk, len);
    if (result == 0) {
        manager->messages_sent++;
        return TRANSPORT_MANAGER_OK;
    } else {
        manager->errors_count++;
        return TRANSPORT_MANAGER_ERROR;
    }
}

const char* transport_manager_result_to_string(transport_manager_result_t result) {
    switch (result) {
        case TRANSPORT_MANAGER_OK: return "OK";
        case TRANSPORT_MANAGER_ERROR: return "Error";
        case TRANSPORT_MANAGER_INVALID_TRANSPORT: return "Invalid transport";
        case TRANSPORT_MANAGER_NOT_CONNECTED: return "Not connected";
        case TRANSPORT_MANAGER_PROTOCOL_ERROR: return "Protocol error";
        default: return "Unknown";
    }
}

int transport_manager_get_connection_count(transport_manager_t* manager) {
    if (!manager || !manager->initialized || !manager->transport) {
        return -1;
    }
    
    // Call transport-specific connection count method if available
    if (manager->transport->get_connection_count) {
        return manager->transport->get_connection_count(manager->transport);
    }
    
    // Fallback: return 1 if connected, 0 if not
    return transport_manager_is_connected(manager) ? 1 : 0;
}

int transport_manager_test_connection_drop(transport_manager_t* manager) {
    if (!manager || !manager->initialized || !manager->transport) {
        return TRANSPORT_MANAGER_ERROR;
    }
    
    // This would call transport-specific connection drop testing
    // For now, just check if we can get connection status
    int connection_count = transport_manager_get_connection_count(manager);
    if (connection_count > 0) {
        printf("Transport manager: %d active connections\n", connection_count);
        return TRANSPORT_MANAGER_OK;
    } else {
        printf("Transport manager: No active connections to test\n");
        return TRANSPORT_MANAGER_NOT_CONNECTED;
    }
}

void transport_manager_get_stats(transport_manager_t* manager, uint32_t* sent, uint32_t* received, uint32_t* errors) {
    if (!manager) return;
    
    if (sent) *sent = manager->messages_sent;
    if (received) *received = manager->messages_received;
    if (errors) *errors = manager->errors_count;
}

int transport_manager_restart_transport(transport_type_t transport_type) {
    // Simplified restart implementation for Phase 4
    // In a full implementation, this would restart the specific transport
    (void)transport_type; // Suppress unused parameter warning
    
    // For now, just simulate successful restart
    // TODO: Implement actual transport restart logic
    return 0; // Success
}

// Global transport manager access functions
void set_global_transport_manager(transport_manager_t* manager) {
    g_global_transport_manager = manager;
}

transport_manager_t* get_global_transport_manager(void) {
    return g_global_transport_manager;
}