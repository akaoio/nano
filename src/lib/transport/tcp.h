#pragma once

#include "base.h"
#include "../protocol/adapter.h"
#include "../core/stream_manager.h"
#include <netinet/in.h>
#include <pthread.h>

// TCP Transport - Multi-client server with streaming support
// Uses MCP Adapter for all protocol logic
// Pure transport layer - no MCP awareness

#define MAX_TCP_CLIENTS 50
#define TCP_BUFFER_SIZE 8192
#define TCP_RESPONSE_SIZE 16384

typedef struct {
    int socket;
    int index;
    struct tcp_transport* transport;
    pthread_t thread;
    bool active;
} tcp_client_data_t;

typedef struct tcp_transport {
    transport_base_t base;
    char* host;
    int port;
    int server_socket;
    tcp_client_data_t clients[MAX_TCP_CLIENTS];
    pthread_t server_thread;
    pthread_mutex_t clients_mutex;
    bool running;
    size_t client_count;
    bool initialized;
    bool connected;
} tcp_transport_t;

typedef struct {
    char* host;
    int port;
    int socket_fd;
    bool is_server;
    bool initialized;
    bool running;
    bool connected;
} tcp_transport_config_t;

// TCP transport functions - Multi-client server support
int tcp_transport_init(void* config);
int tcp_transport_send_raw(const char* data, size_t len);
int tcp_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);
void tcp_transport_shutdown(void);

// Connection management
int tcp_transport_connect(void);
int tcp_transport_disconnect(void);
bool tcp_transport_is_connected(void);

// Multi-client server functions
int tcp_server_start(tcp_transport_t* transport);
int tcp_server_stop(tcp_transport_t* transport);
void* tcp_server_thread(void* arg);
void* tcp_client_handler(void* arg);
int tcp_send_to_all_clients(tcp_transport_t* transport, const char* data, size_t len);

// Streaming support
int tcp_send_stream_chunk(transport_base_t* base, const mcp_stream_chunk_t* chunk);
int tcp_get_connection_count(transport_base_t* base);
bool tcp_test_connection_drop(void);

// Get TCP transport interface
transport_base_t* tcp_transport_get_interface(void);
tcp_transport_t* tcp_transport_create(void);
