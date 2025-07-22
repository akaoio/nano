#ifndef MCP_TRANSPORT_H
#define MCP_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file transport.h
 * @brief MCP Transport Layer Public API
 * 
 * This header defines the public interface for transport layer operations,
 * supporting multiple transport protocols (STDIO, HTTP, WebSocket, TCP, UDP).
 */

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct transport_base transport_base_t;

/**
 * @brief Transport types
 */
typedef enum {
    TRANSPORT_TYPE_STDIO = 0,
    TRANSPORT_TYPE_HTTP,
    TRANSPORT_TYPE_WEBSOCKET,
    TRANSPORT_TYPE_TCP,
    TRANSPORT_TYPE_UDP,
    TRANSPORT_TYPE_COUNT  // Must be last for array sizing
} transport_type_t;

/**
 * @brief Generic transport configuration
 */
typedef struct {
    char* host;
    int port;
    char* path;
    bool initialized;
    bool running;
    bool connected;
    int timeout_ms;
} transport_config_t;

/**
 * @brief HTTP transport configuration
 */
typedef struct {
    char* host;
    int port;
    char* path;
    bool initialized;
    bool running;
    bool connected;
    int timeout_ms;
    bool keep_alive;
} http_transport_config_t;

/**
 * @brief WebSocket transport configuration
 */
typedef struct {
    char* host;
    int port;
    char* path;
    int socket_fd;
    bool initialized;
    bool running;
    bool connected;
    bool mask_frames;
    char sec_key[32];
} ws_transport_config_t;

// Transport initialization functions

/**
 * @brief Initialize HTTP transport
 * @param config HTTP transport configuration
 * @return 0 on success, negative on error
 */
int http_transport_init(void* config);

/**
 * @brief Initialize WebSocket transport
 * @param config WebSocket transport configuration
 * @return 0 on success, negative on error
 */
int ws_transport_init(void* config);

/**
 * @brief Get HTTP transport interface
 * @return Transport interface pointer
 */
transport_base_t* http_transport_get_interface(void);

/**
 * @brief Get WebSocket transport interface
 * @return Transport interface pointer
 */
transport_base_t* ws_transport_get_interface(void);

// Transport operations

/**
 * @brief Send raw data through HTTP transport
 * @param data Data to send
 * @param len Data length
 * @return Number of bytes sent, negative on error
 */
int http_transport_send_raw(const char* data, size_t len);

/**
 * @brief Receive raw data through HTTP transport
 * @param buffer Receive buffer
 * @param buffer_size Buffer size
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes received, negative on error
 */
int http_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);

/**
 * @brief Send raw data through WebSocket transport
 * @param data Data to send
 * @param len Data length
 * @return Number of bytes sent, negative on error
 */
int ws_transport_send_raw(const char* data, size_t len);

/**
 * @brief Receive raw data through WebSocket transport
 * @param buffer Receive buffer
 * @param buffer_size Buffer size
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes received, negative on error
 */
int ws_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);

// Connection management

/**
 * @brief Connect HTTP transport
 * @return 0 on success, negative on error
 */
int http_transport_connect(void);

/**
 * @brief Disconnect HTTP transport
 * @return 0 on success, negative on error
 */
int http_transport_disconnect(void);

/**
 * @brief Check if HTTP transport is connected
 * @return true if connected, false otherwise
 */
bool http_transport_is_connected(void);

/**
 * @brief Connect WebSocket transport
 * @return 0 on success, negative on error
 */
int ws_transport_connect(void);

/**
 * @brief Disconnect WebSocket transport
 * @return 0 on success, negative on error
 */
int ws_transport_disconnect(void);

/**
 * @brief Check if WebSocket transport is connected
 * @return true if connected, false otherwise
 */
bool ws_transport_is_connected(void);

// Shutdown functions

/**
 * @brief Shutdown HTTP transport
 */
void http_transport_shutdown(void);

/**
 * @brief Shutdown WebSocket transport
 */
void ws_transport_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // MCP_TRANSPORT_H