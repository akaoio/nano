#ifndef MCP_SERVER_H
#define MCP_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file server.h
 * @brief MCP Server Public API
 * 
 * This header defines the public interface for the MCP (Model Context Protocol) server.
 * All public functions, types, and constants that clients need to interact with the server.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct mcp_server {
    bool initialized;
    bool running;
    void* internal_data; // Opaque pointer to implementation details
    uint64_t uptime_seconds;
} mcp_server_t;

/**
 * @brief MCP Server configuration structure
 */
typedef struct {
    // Transport configuration
    bool enable_stdio;
    bool enable_tcp;
    bool enable_udp; 
    bool enable_http;
    bool enable_websocket;
    
    // Network settings
    int tcp_port;
    int udp_port;
    int http_port;
    int ws_port;
    char* http_path;
    char* ws_path;
    
    // Server settings
    char* server_name;
    bool enable_streaming;
    bool enable_logging;
    char* log_file;
} mcp_server_config_t;

/**
 * @brief Initialize MCP server with configuration
 * @param server Server instance to initialize
 * @param config Server configuration
 * @return 0 on success, negative on error
 */
int mcp_server_init(mcp_server_t* server, const mcp_server_config_t* config);

/**
 * @brief Start the MCP server
 * @param server Server instance
 * @return 0 on success, negative on error
 */
int mcp_server_start(mcp_server_t* server);

/**
 * @brief Stop the MCP server
 * @param server Server instance
 * @return 0 on success, negative on error
 */
int mcp_server_stop(mcp_server_t* server);

/**
 * @brief Shutdown and cleanup MCP server
 * @param server Server instance
 */
void mcp_server_shutdown(mcp_server_t* server);

/**
 * @brief Get server status string
 * @param server Server instance
 * @return Status string (do not free)
 */
const char* mcp_server_get_status(const mcp_server_t* server);

/**
 * @brief Get server statistics
 * @param server Server instance
 * @param requests Output: Number of requests processed
 * @param responses Output: Number of responses sent
 * @param errors Output: Number of errors handled
 * @param uptime Output: Server uptime in seconds
 */
void mcp_server_get_stats(const mcp_server_t* server, 
                         uint64_t* requests, uint64_t* responses, 
                         uint64_t* errors, uint64_t* uptime);

/**
 * @brief Log a message through the server's logging system
 * @param server Server instance
 * @param level Log level ("info", "warn", "error", "debug")
 * @param message Message to log
 */
void mcp_server_log(mcp_server_t* server, const char* level, const char* message);

#ifdef __cplusplus
}
#endif

#endif // MCP_SERVER_H