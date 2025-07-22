#pragma once

#include "base.h"
#include "../protocol/adapter.h"
#include "../core/stream_manager.h"
#include "mcp/transport.h"
#include <pthread.h>
#include "websocket_stub.h"

// WebSocket Transport - Multi-client server with streaming support
// Uses MCP Adapter for all protocol logic
// Pure transport layer - no MCP awareness

#define MAX_WS_CONNECTIONS 20
#define WS_RESPONSE_SIZE 16384

typedef struct websocket_transport {
    transport_base_t base;
    char* host;
    int port;
    char* path;
    ws_cli_conn_t connections[MAX_WS_CONNECTIONS];
    size_t connection_count;
    pthread_mutex_t connections_mutex;
    pthread_t server_thread;
    bool running;
    bool initialized;
    bool connected;
    char* message_buffer;
    size_t message_buffer_size;
    bool message_ready;
    pthread_mutex_t message_mutex;
} websocket_transport_t;

// WebSocket transport functions - Multi-client server support
int ws_transport_init(void* config);
int ws_transport_send_raw(const char* data, size_t len);
int ws_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);
void ws_transport_shutdown(void);

// Connection management
int ws_transport_connect(void);
int ws_transport_disconnect(void);
bool ws_transport_is_connected(void);

// Multi-client server functions
int websocket_server_start(websocket_transport_t* transport);
int websocket_server_stop(websocket_transport_t* transport);
void* websocket_server_thread(void* arg);
void websocket_on_connect(ws_cli_conn_t connection);
void websocket_on_disconnect(ws_cli_conn_t connection);
void websocket_on_message(ws_cli_conn_t connection, const unsigned char *msg, uint64_t size, int type);
int websocket_send_to_all_clients(websocket_transport_t* transport, const char* data, size_t len);

// Streaming support
int websocket_send_stream_chunk(transport_base_t* base, const mcp_stream_chunk_t* chunk);
int websocket_get_connection_count(transport_base_t* base);
bool websocket_test_connection_drop(void);

// Get WebSocket transport interface
transport_base_t* ws_transport_get_interface(void);
websocket_transport_t* websocket_transport_create(void);
