#define _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#include "udp.h"
#include "common/types.h"
#include "mcp/transport.h"
#include "../protocol/adapter.h"
#include "../core/stream_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>

// Global state for backward compatibility
static udp_transport_config_t g_config = {0};
static struct sockaddr_in g_last_sender = {0};

// New multi-client UDP transport instance
static udp_transport_t g_udp_transport = {0};

// Forward declarations
static int udp_transport_base_init(transport_base_t* base, void* config);
static void udp_transport_base_shutdown(transport_base_t* base);
static int udp_transport_base_connect(transport_base_t* base);
static int udp_transport_base_disconnect(transport_base_t* base);
static int udp_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int udp_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int udp_transport_base_is_connected(transport_base_t* base);

// Multi-client UDP server thread
void* udp_server_thread(void* arg) {
    udp_transport_t* transport = (udp_transport_t*)arg;
    
    // Create UDP socket
    transport->server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (transport->server_socket < 0) {
        printf("UDP: Failed to create server socket: %s\n", strerror(errno));
        return NULL;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(transport->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("UDP: Failed to set socket options: %s\n", strerror(errno));
        close(transport->server_socket);
        return NULL;
    }
    
    // Bind socket
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(transport->port);
    
    if (bind(transport->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("UDP: Failed to bind to port %d: %s\n", transport->port, strerror(errno));
        close(transport->server_socket);
        return NULL;
    }
    
    printf("UDP: Server listening on %s:%d\n", transport->host ? transport->host : "0.0.0.0", transport->port);
    
    char buffer[UDP_BUFFER_SIZE];
    char response_buffer[UDP_RESPONSE_SIZE];
    
    while (transport->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Receive packet from client
        ssize_t bytes_received = recvfrom(transport->server_socket, buffer, sizeof(buffer) - 1, 0,
                                        (struct sockaddr*)&client_addr, &client_len);
        if (bytes_received <= 0) {
            if (transport->running) {
                printf("UDP: Failed to receive packet: %s\n", strerror(errno));
            }
            continue;
        }
        
        buffer[bytes_received] = '\0';
        printf("UDP: Received %zd bytes from %s:%d: %.100s%s\n",
               bytes_received, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
               buffer, bytes_received > 100 ? "..." : "");
        
        // Register client for streaming
        udp_register_client(transport, &client_addr);
        
        // Process request through MCP adapter
        size_t response_size = sizeof(response_buffer);
        // For now, use simple echo response until proper MCP adapter integration
        snprintf(response_buffer, sizeof(response_buffer), 
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"status\":\"received\",\"bytes\":%zd}}", 
                bytes_received);
        response_size = strlen(response_buffer);
        int result = 0; // Success
        
        if (result == 0 && response_size > 0) {
            // Send response back to client
            ssize_t bytes_sent = sendto(transport->server_socket, response_buffer, response_size, 0,
                                      (struct sockaddr*)&client_addr, client_len);
            if (bytes_sent < 0) {
                printf("UDP: Failed to send response: %s\n", strerror(errno));
            } else {
                printf("UDP: Sent %zd bytes to %s:%d\n", bytes_sent, 
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            }
        } else {
            // Send error response
            const char* error_response = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32603,\"message\":\"Internal error\"},\"id\":null}\\n";
            ssize_t bytes_sent = sendto(transport->server_socket, error_response, strlen(error_response), 0,
                                      (struct sockaddr*)&client_addr, client_len);
            if (bytes_sent < 0) {
                printf("UDP: Failed to send error response: %s\n", strerror(errno));
            }
        }
    }
    
    close(transport->server_socket);
    transport->server_socket = -1;
    return NULL;
}

// Register client for streaming (UDP clients are registered when they send packets)
void udp_register_client(udp_transport_t* transport, struct sockaddr_in* client_addr) {
    if (!transport || !client_addr) {
        return;
    }
    
    pthread_mutex_lock(&transport->clients_mutex);
    
    // Check if client already registered
    for (size_t i = 0; i < transport->client_count; i++) {
        if (memcmp(&transport->clients[i], client_addr, sizeof(struct sockaddr_in)) == 0) {
            pthread_mutex_unlock(&transport->clients_mutex);
            return; // Already registered
        }
    }
    
    // Add new client if space available
    if (transport->client_count < MAX_UDP_CLIENTS) {
        memcpy(&transport->clients[transport->client_count], client_addr, sizeof(struct sockaddr_in));
        transport->client_count++;
        printf("UDP: Registered new client %s:%d (total: %zu)\n",
               inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port), transport->client_count);
    } else {
        printf("UDP: Maximum clients reached (%d), cannot register %s:%d\n",
               MAX_UDP_CLIENTS, inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    }
    
    pthread_mutex_unlock(&transport->clients_mutex);
}

// Start UDP server
int udp_server_start(udp_transport_t* transport) {
    if (!transport || transport->running) {
        return -1;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&transport->clients_mutex, NULL) != 0) {
        printf("UDP: Failed to initialize clients mutex: %s\n", strerror(errno));
        return -1;
    }
    
    transport->running = true;
    transport->client_count = 0;
    
    // Create server thread
    int result = pthread_create(&transport->server_thread, NULL, udp_server_thread, transport);
    if (result != 0) {
        printf("UDP: Failed to create server thread: %s\n", strerror(result));
        transport->running = false;
        pthread_mutex_destroy(&transport->clients_mutex);
        return -1;
    }
    
    // Give server thread time to bind to port
    usleep(100000); // 100ms
    
    transport->connected = true;
    return 0;
}

// Stop UDP server
int udp_server_stop(udp_transport_t* transport) {
    if (!transport || !transport->running) {
        return 0;
    }
    
    printf("UDP: Stopping server...\n");
    transport->running = false;
    
    // Close server socket to unblock recvfrom()
    if (transport->server_socket >= 0) {
        close(transport->server_socket);
        transport->server_socket = -1;
    }
    
    // Wait for server thread
    pthread_join(transport->server_thread, NULL);
    
    // Clear client list
    pthread_mutex_lock(&transport->clients_mutex);
    transport->client_count = 0;
    pthread_mutex_unlock(&transport->clients_mutex);
    
    pthread_mutex_destroy(&transport->clients_mutex);
    transport->connected = false;
    
    printf("UDP: Server stopped\n");
    return 0;
}

// Send data to all registered clients
int udp_send_to_all_clients(udp_transport_t* transport, const char* data, size_t len) {
    if (!transport || !data || len == 0) {
        return -1;
    }
    
    int clients_sent = 0;
    
    pthread_mutex_lock(&transport->clients_mutex);
    for (size_t i = 0; i < transport->client_count; i++) {
        ssize_t bytes_sent = sendto(transport->server_socket, data, len, 0,
                                  (struct sockaddr*)&transport->clients[i], sizeof(transport->clients[i]));
        if (bytes_sent > 0) {
            clients_sent++;
        } else {
            printf("UDP: Failed to send to client %s:%d: %s\n",
                   inet_ntoa(transport->clients[i].sin_addr), ntohs(transport->clients[i].sin_port),
                   strerror(errno));
        }
    }
    pthread_mutex_unlock(&transport->clients_mutex);
    
    return clients_sent;
}

// Streaming support - send chunk to all clients
int udp_send_stream_chunk(transport_base_t* base, const mcp_stream_chunk_t* chunk) {
    udp_transport_t* transport = (udp_transport_t*)base;
    if (!transport || !chunk) {
        return -1;
    }
    
    // Format chunk as JSON-RPC response
    char chunk_json[4096];
    int result = mcp_adapter_format_stream_chunk(chunk, chunk_json, sizeof(chunk_json));
    if (result != MCP_ADAPTER_OK) {
        printf("UDP: Failed to format stream chunk\n");
        return -1;
    }
    
    // Send to all registered clients
    int clients_sent = udp_send_to_all_clients(transport, chunk_json, strlen(chunk_json));
    printf("UDP: Sent stream chunk to %d clients\n", clients_sent);
    
    return clients_sent > 0 ? 0 : -1;
}

// Get connection count
int udp_get_connection_count(transport_base_t* base) {
    udp_transport_t* transport = (udp_transport_t*)base;
    if (!transport) {
        return -1;
    }
    
    pthread_mutex_lock(&transport->clients_mutex);
    size_t count = transport->client_count;
    pthread_mutex_unlock(&transport->clients_mutex);
    
    return (int)count;
}

// Test connection drop simulation
bool udp_test_connection_drop(void) {
    udp_transport_t* transport = &g_udp_transport;
    
    if (!transport->running || transport->client_count == 0) {
        return false; // No connections to drop
    }
    
    // Remove the last registered client to simulate connection drop
    pthread_mutex_lock(&transport->clients_mutex);
    if (transport->client_count > 0) {
        transport->client_count--;
        printf("UDP: Simulated connection drop - removed client (remaining: %zu)\n", transport->client_count);
        pthread_mutex_unlock(&transport->clients_mutex);
        return true;
    }
    pthread_mutex_unlock(&transport->clients_mutex);
    
    return false;
}

// Create new UDP transport instance
udp_transport_t* udp_transport_create(void) {
    udp_transport_t* transport = malloc(sizeof(udp_transport_t));
    if (!transport) {
        return NULL;
    }
    
    memset(transport, 0, sizeof(udp_transport_t));
    
    // Initialize base transport interface
    strncpy(transport->base.name, "udp", sizeof(transport->base.name) - 1);
    transport->base.name[sizeof(transport->base.name) - 1] = '\0';
    transport->base.state = TRANSPORT_STATE_DISCONNECTED;
    transport->base.initialized = false;
    transport->base.running = false;
    transport->base.init = udp_transport_base_init;
    transport->base.shutdown = udp_transport_base_shutdown;
    transport->base.connect = udp_transport_base_connect;
    transport->base.disconnect = udp_transport_base_disconnect;
    transport->base.send = udp_transport_base_send;
    transport->base.recv = udp_transport_base_recv;
    transport->base.is_connected = udp_transport_base_is_connected;
    transport->base.send_stream_chunk = (int (*)(struct transport_base*, const void*))udp_send_stream_chunk;
    transport->base.get_connection_count = udp_get_connection_count;
    transport->base.impl_data = transport;
    
    transport->server_socket = -1;
    transport->enable_retry = true;
    transport->max_retries = 3;
    transport->retry_timeout_ms = 1000;
    
    return transport;
}

// Legacy functions for backward compatibility
int udp_transport_init(void* config) {
    if (!config) return -1;
    
    udp_transport_config_t* cfg = (udp_transport_config_t*)config;
    g_config = *cfg;
    
    // Copy host string
    if (cfg->host) {
        g_config.host = strdup(cfg->host);
    }
    
    // UDP reliability defaults
    g_config.enable_retry = cfg->enable_retry;
    g_config.max_retries = cfg->max_retries ? cfg->max_retries : 3;
    g_config.retry_timeout_ms = cfg->retry_timeout_ms ? cfg->retry_timeout_ms : 1000;
    
    g_config.connected = false;
    g_config.socket_fd = -1;
    g_config.initialized = true;
    g_config.running = true;
    
    // Initialize new transport
    g_udp_transport.host = g_config.host ? strdup(g_config.host) : NULL;
    g_udp_transport.port = g_config.port;
    g_udp_transport.enable_retry = g_config.enable_retry;
    g_udp_transport.max_retries = g_config.max_retries;
    g_udp_transport.retry_timeout_ms = g_config.retry_timeout_ms;
    g_udp_transport.initialized = true;
    
    return 0;
}

int udp_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0) {
        return -1;
    }
    
    // Use new multi-client implementation if server mode
    if (g_udp_transport.running) {
        return udp_send_to_all_clients(&g_udp_transport, data, len) > 0 ? 0 : -1;
    }
    
    // Legacy implementation for client mode
    if (!g_config.connected) {
        return -1;
    }
    
    // Send to the last sender address (for server responses) or config address (for client)
    struct sockaddr_in* target_addr = &g_config.addr;
    if (g_last_sender.sin_family == AF_INET && g_last_sender.sin_port != 0) {
        target_addr = &g_last_sender;  // Respond to last sender
    }
    
    // Use retry mechanism for reliability if enabled
    if (g_config.enable_retry) {
        return udp_transport_send_with_retry_to_addr(data, len, target_addr);
    } else {
        ssize_t sent = sendto(g_config.socket_fd, data, len, 0,
                             (struct sockaddr*)target_addr, sizeof(*target_addr));
        return (sent == (ssize_t)len) ? 0 : -1;
    }
}

int udp_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_config.initialized || !buffer || buffer_size == 0) {
        return -1;
    }
    
    // Use legacy implementation for now - the new multi-client server handles requests in its own thread
    if (!g_config.connected) {
        return -1;
    }
    
    // Use select for timeout
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(g_config.socket_fd, &readfds);
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(g_config.socket_fd + 1, &readfds, NULL, NULL, &timeout);
    if (result <= 0) {
        return -1; // Timeout or error
    }
    
    // Read data and store sender address
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ssize_t received = recvfrom(g_config.socket_fd, buffer, buffer_size - 1, 0,
                               (struct sockaddr*)&from_addr, &from_len);
    
    if (received <= 0) {
        return -1;
    }
    
    // Store sender address for response
    g_last_sender = from_addr;
    
    buffer[received] = '\0';
    return 0;
}

int udp_transport_connect(void) {
    if (!g_config.initialized) return -1;
    
    if (g_config.connected) return 0; // Already connected
    
    // Start new multi-client server
    g_udp_transport.host = g_config.host ? strdup(g_config.host) : NULL;
    g_udp_transport.port = g_config.port;
    
    if (udp_server_start(&g_udp_transport) == 0) {
        printf("✅ UDP multi-client server started on port %d\n", g_config.port);
    }
    
    // Legacy setup for backward compatibility
    // Create socket
    g_config.socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_config.socket_fd < 0) {
        return -1;
    }
    
    // Setup address
    memset(&g_config.addr, 0, sizeof(g_config.addr));
    g_config.addr.sin_family = AF_INET;
    g_config.addr.sin_port = htons(g_config.port);
    
    if (g_config.host) {
        if (inet_pton(AF_INET, g_config.host, &g_config.addr.sin_addr) <= 0) {
            close(g_config.socket_fd);
            return -1;
        }
    } else {
        g_config.addr.sin_addr.s_addr = INADDR_ANY;
    }
    
    // Bind socket for receiving
    if (bind(g_config.socket_fd, (struct sockaddr*)&g_config.addr, sizeof(g_config.addr)) < 0) {
        printf("UDP bind failed on port %d: %s\n", g_config.port, strerror(errno));
        close(g_config.socket_fd);
        return -1;
    }
    
    printf("✅ UDP legacy server bound to port %d\n", g_config.port);
    g_config.connected = true;
    return 0;
}

int udp_transport_disconnect(void) {
    if (!g_config.initialized) return -1;
    
    // Stop multi-client server
    udp_server_stop(&g_udp_transport);
    
    if (g_config.socket_fd >= 0) {
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
    }
    
    g_config.connected = false;
    return 0;
}

bool udp_transport_is_connected(void) {
    return g_config.initialized && (g_config.connected || g_udp_transport.connected);
}

void udp_transport_shutdown(void) {
    udp_transport_disconnect();
    
    if (g_config.host) {
        free(g_config.host);
        g_config.host = NULL;
    }
    
    if (g_udp_transport.host) {
        free(g_udp_transport.host);
        g_udp_transport.host = NULL;
    }
    
    g_config.running = false;
    g_config.initialized = false;
    g_udp_transport.initialized = false;
}

// UDP reliability - send with retry
int udp_transport_send_with_retry(const char* data, size_t len) {
    return udp_transport_send_with_retry_to_addr(data, len, &g_config.addr);
}

// UDP reliability - send with retry to specific address
int udp_transport_send_with_retry_to_addr(const char* data, size_t len, struct sockaddr_in* addr) {
    if (!g_config.initialized || !data || len == 0 || !addr) return -1;
    
    int attempts = 0;
    while (attempts < g_config.max_retries) {
        ssize_t sent = sendto(g_config.socket_fd, data, len, 0,
                             (struct sockaddr*)addr, sizeof(*addr));
        
        if (sent == (ssize_t)len) {
            return 0; // Success
        }
        
        attempts++;
        if (attempts < g_config.max_retries) {
            // Wait before retry (convert ms to microseconds for usleep)
            usleep(g_config.retry_timeout_ms * 1000);
        }
    }
    
    return -1; // All retries failed
}

// Transport base interface implementations
static int udp_transport_base_init(transport_base_t* base, void* config) {
    return udp_transport_init(config);
}

static void udp_transport_base_shutdown(transport_base_t* base) {
    udp_transport_shutdown();
}

static int udp_transport_base_connect(transport_base_t* base) {
    return udp_transport_connect();
}

static int udp_transport_base_disconnect(transport_base_t* base) {
    return udp_transport_disconnect();
}

static int udp_transport_base_send(transport_base_t* base, const char* data, size_t len) {
    return udp_transport_send_raw(data, len);
}

static int udp_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms) {
    return udp_transport_recv_raw(buffer, buffer_size, timeout_ms);
}

static int udp_transport_base_is_connected(transport_base_t* base) {
    return udp_transport_is_connected() ? 1 : 0;
}

static transport_base_t g_udp_transport_base = {
    .name = "udp",
    .state = TRANSPORT_STATE_DISCONNECTED,
    .initialized = false,
    .running = false,
    .init = udp_transport_base_init,
    .shutdown = udp_transport_base_shutdown,
    .connect = udp_transport_base_connect,
    .disconnect = udp_transport_base_disconnect,
    .send = udp_transport_base_send,
    .recv = udp_transport_base_recv,
    .is_connected = udp_transport_base_is_connected,
    .send_stream_chunk = (int (*)(struct transport_base*, const void*))udp_send_stream_chunk,
    .get_connection_count = udp_get_connection_count,
    .impl_data = &g_udp_transport,
    .on_error = NULL,
    .on_state_change = NULL
};

transport_base_t* udp_transport_get_interface(void) {
    return &g_udp_transport_base;
}