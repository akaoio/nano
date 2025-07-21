#include "server.h"
#include "settings_global.h"
#include "mcp/transport.h"
#include "common/types.h"
#include "../protocol/adapter.h"
#include "../transport/manager.h"
#include "../transport/stdio.h"
#include "../transport/tcp.h"
#include "../transport/udp.h"
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

static mcp_server_internal_t g_server = {0};

int mcp_server_internal_init(mcp_server_internal_t* server, const mcp_server_config_t* config) {
    if (!server || !config) return -1;
    
    // Validate configuration
    if (mcp_server_internal_validate_config(config) != 0) {
        return -1;
    }
    
    // Initialize server structure
    memset(server, 0, sizeof(mcp_server_internal_t));
    
    // Set server info
    strncpy(server->server_name, config->server_name ? config->server_name : "MCP-Server", 
            sizeof(server->server_name) - 1);
    strncpy(server->version, "1.0.0", sizeof(server->version) - 1);
    server->default_port = config->tcp_port ? config->tcp_port : 8080;
    
    // Initialize MCP adapter
    server->mcp_adapter = &g_mcp_adapter;
    if (mcp_adapter_init(server->mcp_adapter) != MCP_ADAPTER_OK) {
        mcp_server_internal_log(server, "ERROR", "Failed to initialize MCP adapter");
        return -1;
    }
    
    // Allocate transport managers array
    size_t max_transports = 5; // STDIO, TCP, UDP, HTTP, WebSocket
    server->transport_managers = calloc(max_transports, sizeof(transport_manager_t));
    if (!server->transport_managers) {
        mcp_server_internal_log(server, "ERROR", "Failed to allocate transport managers");
        return -1;
    }
    
    server->transport_count = 0;
    
    // Initialize enabled transports
    if (config->enable_stdio) {
        stdio_transport_config_t stdio_config = {
            .log_to_stderr = config->enable_logging,
            .line_buffered = true
        };
        
        transport_base_t* stdio_transport = stdio_transport_get_interface();
        if (mcp_server_internal_add_transport(server, stdio_transport, &stdio_config) == 0) {
            mcp_server_internal_log(server, "INFO", "STDIO transport initialized");
        }
    }
    
    if (config->enable_tcp) {
        tcp_transport_config_t tcp_config = {
            .host = strdup(SETTING_TRANSPORT(tcp, host) ? SETTING_TRANSPORT(tcp, host) : "0.0.0.0"),
            .port = SETTING_TRANSPORT(tcp, port) ? SETTING_TRANSPORT(tcp, port) : config->tcp_port,
            .is_server = true
        };
        // Note: TCP timeout_ms and max_retries from JSON are not used by current TCP transport implementation
        
        transport_base_t* tcp_transport = tcp_transport_get_interface();
        if (mcp_server_internal_add_transport(server, tcp_transport, &tcp_config) == 0) {
            mcp_server_internal_log(server, "INFO", "TCP transport initialized");
        }
    }
    
    if (config->enable_udp) {
        udp_transport_config_t udp_config = {
            .host = strdup(SETTING_TRANSPORT(udp, host) ? SETTING_TRANSPORT(udp, host) : "0.0.0.0"),
            .port = SETTING_TRANSPORT(udp, port) ? SETTING_TRANSPORT(udp, port) : (config->udp_port ? config->udp_port : 8081),
            .enable_retry = true,  // Enable UDP reliability features
            .max_retries = SETTING_TRANSPORT(udp, max_retries) ? SETTING_TRANSPORT(udp, max_retries) : 3,
            .retry_timeout_ms = SETTING_TRANSPORT(udp, timeout_ms) ? SETTING_TRANSPORT(udp, timeout_ms) : 5000
        };
        
        transport_base_t* udp_transport = udp_transport_get_interface();
        if (mcp_server_internal_add_transport(server, udp_transport, &udp_config) == 0) {
            mcp_server_internal_log(server, "INFO", "UDP transport initialized");
        }
    }
    
    if (config->enable_http) {
        http_transport_config_t http_config = {
            .host = strdup(SETTING_TRANSPORT(http, host) ? SETTING_TRANSPORT(http, host) : "0.0.0.0"),
            .port = SETTING_TRANSPORT(http, port) ? SETTING_TRANSPORT(http, port) : (config->http_port ? config->http_port : 8082),
            .path = strdup(SETTING_TRANSPORT(http, path) ? SETTING_TRANSPORT(http, path) : (config->http_path ? config->http_path : "/")),
            .timeout_ms = SETTING_TRANSPORT(http, timeout_ms) ? SETTING_TRANSPORT(http, timeout_ms) : 30000,
            .keep_alive = SETTING_TRANSPORT(http, keep_alive)
        };
        
        transport_base_t* http_transport = http_transport_get_interface();
        if (mcp_server_internal_add_transport(server, http_transport, &http_config) == 0) {
            mcp_server_internal_log(server, "INFO", "HTTP transport initialized");
        }
    }
    
    if (config->enable_websocket) {
        ws_transport_config_t ws_config = {
            .host = strdup(SETTING_TRANSPORT(websocket, host) ? SETTING_TRANSPORT(websocket, host) : "0.0.0.0"),
            .port = SETTING_TRANSPORT(websocket, port) ? SETTING_TRANSPORT(websocket, port) : (config->ws_port ? config->ws_port : 8083),
            .path = strdup(SETTING_TRANSPORT(websocket, path) ? SETTING_TRANSPORT(websocket, path) : (config->ws_path ? config->ws_path : "/"))
        };
        
        transport_base_t* ws_transport = ws_transport_get_interface();
        if (mcp_server_internal_add_transport(server, ws_transport, &ws_config) == 0) {
            mcp_server_internal_log(server, "INFO", "WebSocket transport initialized");
        }
    }
    
    server->initialized = true;
    mcp_server_internal_log(server, "INFO", "MCP Server initialized successfully");
    
    return 0;
}

int mcp_server_internal_start(mcp_server_internal_t* server) {
    if (!server || !server->initialized) return -1;
    
    mcp_server_internal_log(server, "INFO", "Starting MCP Server...");
    
    // Connect all transports
    for (size_t i = 0; i < server->transport_count; i++) {
        transport_manager_t* manager = &server->transport_managers[i];
        if (transport_manager_connect(manager) == TRANSPORT_MANAGER_OK) {
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Transport %zu connected successfully", i);
            mcp_server_internal_log(server, "INFO", log_msg);
        } else {
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "Failed to connect transport %zu", i);
            mcp_server_internal_log(server, "WARNING", log_msg);
        }
    }
    
    server->running = true;
    mcp_server_internal_log(server, "INFO", "MCP Server started and listening for connections");
    
    return 0;
}

int mcp_server_internal_run_event_loop(mcp_server_internal_t* server) {
    if (!server || !server->running) return -1;
    
    mcp_server_internal_log(server, "INFO", "Starting request processing loop...");
    
    while (server->running) {
        // Poll all transports for incoming requests
        for (size_t i = 0; i < server->transport_count; i++) {
            transport_manager_t* manager = &server->transport_managers[i];
            
            if (!transport_manager_is_connected(manager)) {
                continue;
            }
            
            // Use dedicated buffers for this transport to prevent contamination
            char* request_buffer = manager->request_buffer;
            char* response_buffer = manager->response_buffer;
            
            // Try to receive a request (use longer timeout for HTTP/WebSocket)
            int timeout_ms = 100; // Default for TCP/UDP/STDIO
            if (manager->transport && manager->transport->name) {
                if (strcmp(manager->transport->name, "http") == 0 || 
                    strcmp(manager->transport->name, "websocket") == 0) {
                    timeout_ms = 1000; // 1 second for HTTP/WebSocket to handle connection setup
                }
            }
            int result = transport_manager_recv_mcp_message(manager, request_buffer, manager->buffer_size, timeout_ms);
            
            if (result == TRANSPORT_MANAGER_OK) {
                // Process the request through MCP layer
                mcp_server_internal_process_request(server, request_buffer, response_buffer, manager->buffer_size);
                
                // Ensure response ends with newline for MCP compliance
                size_t len = strlen(response_buffer);
                if (len > 0 && response_buffer[len-1] != '\n') {
                    if (len < manager->buffer_size - 2) {
                        response_buffer[len] = '\n';
                        response_buffer[len + 1] = '\0';
                    }
                }
                
                // Send response through transport
                manager->transport->send(manager->transport, response_buffer, strlen(response_buffer));
                
                char log_msg[128];
                snprintf(log_msg, sizeof(log_msg), "Processed request on transport %zu", i);
                mcp_server_internal_log(server, "INFO", log_msg);
            }
        }
        
        // Small sleep to prevent busy waiting
        // Use select for precise timing without platform issues
        struct timeval tv = {0, 10000}; // 10ms
        select(0, NULL, NULL, NULL, &tv);
    }
    
    mcp_server_internal_log(server, "INFO", "Request processing loop stopped");
    return 0;
}

int mcp_server_internal_stop(mcp_server_internal_t* server) {
    if (!server || !server->running) return -1;
    
    mcp_server_internal_log(server, "INFO", "Stopping MCP Server...");
    
    // Disconnect all transports
    for (size_t i = 0; i < server->transport_count; i++) {
        transport_manager_disconnect(&server->transport_managers[i]);
    }
    
    server->running = false;
    mcp_server_internal_log(server, "INFO", "MCP Server stopped");
    
    return 0;
}

void mcp_server_internal_shutdown(mcp_server_internal_t* server) {
    if (!server) return;
    
    if (server->running) {
        mcp_server_internal_stop(server);
    }
    
    mcp_server_internal_log(server, "INFO", "Shutting down MCP Server...");
    
    // Shutdown all transport managers
    for (size_t i = 0; i < server->transport_count; i++) {
        transport_manager_shutdown(&server->transport_managers[i]);
    }
    
    // Free transport managers array
    if (server->transport_managers) {
        free(server->transport_managers);
        server->transport_managers = NULL;
    }
    
    // Shutdown MCP adapter (note: global instance)
    if (server->mcp_adapter && server->mcp_adapter->initialized) {
        mcp_adapter_shutdown(server->mcp_adapter);
    }
    
    server->transport_count = 0;
    server->initialized = false;
    
    mcp_server_internal_log(server, "INFO", "MCP Server shutdown complete");
}

int mcp_server_internal_process_request(mcp_server_internal_t* server, const char* raw_request, char* response, size_t response_size) {
    if (!server || !server->initialized || !raw_request || !response || response_size == 0) {
        return -1;
    }
    
    // Parse request using MCP adapter
    mcp_request_t request;
    if (mcp_adapter_parse_request(raw_request, &request) != MCP_ADAPTER_OK) {
        server->errors_handled++;
        return mcp_adapter_format_error(0, -32700, "Parse error", response, response_size);
    }
    
    // Validate request
    if (mcp_adapter_validate_request(&request) != MCP_ADAPTER_OK) {
        server->errors_handled++;
        return mcp_adapter_format_error(request.request_id, -32600, "Invalid request", response, response_size);
    }
    
    // Process request through MCP adapter
    mcp_response_t mcp_response;
    if (mcp_adapter_process_request(&request, &mcp_response) == MCP_ADAPTER_OK) {
        // Format response
        int format_result = mcp_adapter_format_response(&mcp_response, response, response_size);
        if (format_result == MCP_ADAPTER_OK) {
            server->requests_processed++;
            server->responses_sent++;
            return 0;
        }
    }
    
    server->errors_handled++;
    return mcp_adapter_format_error(request.request_id, -32603, "Internal error", response, response_size);
}

int mcp_server_internal_add_transport(mcp_server_internal_t* server, transport_base_t* transport, void* config) {
    if (!server || !transport || server->transport_count >= 5) {
        return -1;
    }
    
    transport_manager_t* manager = &server->transport_managers[server->transport_count];
    
    if (transport_manager_init(manager, transport) == TRANSPORT_MANAGER_OK) {
        if (transport->init && transport->init(transport, config) == 0) {
            server->transport_count++;
            return 0;
        }
    }
    
    return -1;
}

void mcp_server_internal_get_stats(mcp_server_internal_t* server, uint64_t* requests, uint64_t* responses, uint64_t* errors, uint64_t* uptime) {
    if (!server) return;
    
    if (requests) *requests = server->requests_processed;
    if (responses) *responses = server->responses_sent;
    if (errors) *errors = server->errors_handled;
    if (uptime) *uptime = server->uptime_seconds;
}

const char* mcp_server_internal_get_status(mcp_server_internal_t* server) {
    if (!server) return "Unknown";
    if (!server->initialized) return "Not initialized";
    if (server->running) return "Running";
    return "Stopped";
}

int mcp_server_internal_validate_config(const mcp_server_config_t* config) {
    if (!config) return -1;
    
    // Check that at least one transport is enabled
    if (!config->enable_stdio && !config->enable_tcp && !config->enable_udp && 
        !config->enable_http && !config->enable_websocket) {
        return -1;
    }
    
    // Validate ports
    if (config->enable_tcp && (config->tcp_port == 0 || config->tcp_port > 65535)) {
        return -1;
    }
    
    return 0;
}

void mcp_server_internal_log(mcp_server_internal_t* server, const char* level, const char* message) {
    if (!level || !message) return;
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(stderr, "[%s] [%s] %s\n", timestamp, level, message);
    fflush(stderr);
}