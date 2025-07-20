#include "manager.h"
#include "common/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int transport_manager_init(transport_manager_t* manager, transport_base_t* transport) {
    if (!manager || !transport) return TRANSPORT_MANAGER_ERROR;
    
    memset(manager, 0, sizeof(transport_manager_t));
    manager->transport = transport;
    
    // Initialize MCP adapter (use global instance)
    manager->mcp_adapter = &g_mcp_adapter;
    if (!manager->mcp_adapter->initialized) {
        if (mcp_adapter_init(manager->mcp_adapter) != MCP_ADAPTER_OK) {
            return TRANSPORT_MANAGER_ERROR;
        }
    }
    
    // Initialize transport
    if (transport->init && transport->init(transport, NULL) != 0) {
        return TRANSPORT_MANAGER_ERROR;
    }
    
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
    
    // Use MCP adapter to format the request
    char formatted_data[8192];
    mcp_response_t temp_response;
    memset(&temp_response, 0, sizeof(temp_response));
    
    // Convert request to response format for formatting
    strncpy(temp_response.request_id, request->request_id, sizeof(temp_response.request_id) - 1);
    temp_response.is_success = true;
    strncpy(temp_response.result, request->params, sizeof(temp_response.result) - 1);
    
    // Format as JSON-RPC request (reverse engineer from response format)
    char request_json[8192];
    snprintf(request_json, sizeof(request_json), 
        "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s,\"id\":%s}",
        request->method, request->params, request->request_id);
    
    // Add newline for MCP compliance
    size_t len = strlen(request_json);
    if (len < sizeof(request_json) - 2) {
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
    
    // Use MCP adapter to format the response
    char formatted_data[8192];
    if (mcp_adapter_format_response(response, formatted_data, sizeof(formatted_data)) != MCP_ADAPTER_OK) {
        manager->errors_count++;
        return TRANSPORT_MANAGER_PROTOCOL_ERROR;
    }
    
    // Add newline for MCP compliance
    size_t len = strlen(formatted_data);
    if (len < sizeof(formatted_data) - 2) {
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
    
    // Use MCP adapter to format stream chunk
    char formatted_chunk[8192];
    if (mcp_adapter_format_stream_chunk(chunk, formatted_chunk, sizeof(formatted_chunk)) != MCP_ADAPTER_OK) {
        manager->errors_count++;
        return TRANSPORT_MANAGER_PROTOCOL_ERROR;
    }
    
    // Add newline for MCP compliance
    size_t len = strlen(formatted_chunk);
    if (len < sizeof(formatted_chunk) - 2) {
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

void transport_manager_get_stats(transport_manager_t* manager, uint32_t* sent, uint32_t* received, uint32_t* errors) {
    if (!manager) return;
    
    if (sent) *sent = manager->messages_sent;
    if (received) *received = manager->messages_received;
    if (errors) *errors = manager->errors_count;
}