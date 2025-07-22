#pragma once

#include "base.h"
#include "../protocol/adapter.h"
#include "../core/stream_manager.h"
#include <netinet/in.h>
#include <pthread.h>

// UDP Transport - Multi-client server with streaming support
// Uses MCP Adapter for all protocol logic
// Pure transport layer - no MCP awareness

#define MAX_UDP_CLIENTS 100
#define UDP_BUFFER_SIZE 8192
#define UDP_RESPONSE_SIZE 16384

typedef struct udp_transport {
    transport_base_t base;
    char* host;
    int port;
    int server_socket;
    pthread_t server_thread;
    pthread_mutex_t clients_mutex;
    struct sockaddr_in clients[MAX_UDP_CLIENTS];
    size_t client_count;
    bool running;
    bool initialized;
    bool connected;
    
    // UDP-specific reliability features
    bool enable_retry;
    int max_retries;
    int retry_timeout_ms;
} udp_transport_t;

typedef struct {
    char* host;
    int port;
    int socket_fd;
    struct sockaddr_in addr;
    bool initialized;
    bool running;
    bool connected;
    
    // UDP-specific reliability features (transport-level only)
    bool enable_retry;
    int max_retries;
    int retry_timeout_ms;
} udp_transport_config_t;

// UDP transport functions - Multi-client server support
int udp_transport_init(void* config);
int udp_transport_send_raw(const char* data, size_t len);
int udp_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms);
void udp_transport_shutdown(void);

// Connection management
int udp_transport_connect(void);
int udp_transport_disconnect(void);
bool udp_transport_is_connected(void);

// Multi-client server functions
int udp_server_start(udp_transport_t* transport);
int udp_server_stop(udp_transport_t* transport);
void* udp_server_thread(void* arg);
void udp_register_client(udp_transport_t* transport, struct sockaddr_in* client_addr);
int udp_send_to_all_clients(udp_transport_t* transport, const char* data, size_t len);

// UDP reliability features (transport-level only)
int udp_transport_send_with_retry(const char* data, size_t len);
int udp_transport_send_with_retry_to_addr(const char* data, size_t len, struct sockaddr_in* addr);

// Streaming support
int udp_send_stream_chunk(transport_base_t* base, const mcp_stream_chunk_t* chunk);
int udp_get_connection_count(transport_base_t* base);
bool udp_test_connection_drop(void);

// Get UDP transport interface
transport_base_t* udp_transport_get_interface(void);
udp_transport_t* udp_transport_create(void);
