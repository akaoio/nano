#define _DEFAULT_SOURCE
#include "tcp.h"
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
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>

// Global state for backward compatibility
static tcp_transport_config_t g_config = {0};
static int g_client_fd = -1;
static bool g_client_has_data = false;
static bool g_response_sent = false;

// New multi-client TCP transport instance
static tcp_transport_t g_tcp_transport = {0};

// Forward declarations
static int tcp_transport_base_init(transport_base_t* base, void* config);
static void tcp_transport_base_shutdown(transport_base_t* base);
static int tcp_transport_base_connect(transport_base_t* base);
static int tcp_transport_base_disconnect(transport_base_t* base);
static int tcp_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int tcp_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int tcp_transport_base_is_connected(transport_base_t* base);

// Multi-client TCP server thread
void* tcp_server_thread(void* arg) {
    tcp_transport_t* transport = (tcp_transport_t*)arg;
    
    // Create server socket
    transport->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (transport->server_socket < 0) {
        printf("TCP: Failed to create server socket: %s\n", strerror(errno));
        return NULL;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(transport->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("TCP: Failed to set socket options: %s\n", strerror(errno));
        close(transport->server_socket);
        return NULL;
    }
    
    // Bind and listen
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(transport->port);
    
    if (bind(transport->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("TCP: Failed to bind to port %d: %s\n", transport->port, strerror(errno));
        close(transport->server_socket);
        return NULL;
    }
    
    if (listen(transport->server_socket, MAX_TCP_CLIENTS) < 0) {
        printf("TCP: Failed to listen: %s\n", strerror(errno));
        close(transport->server_socket);
        return NULL;
    }
    
    printf("TCP: Server listening on %s:%d\n", transport->host ? transport->host : "0.0.0.0", transport->port);
    
    // Accept connections
    while (transport->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(transport->server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (transport->running) {
                printf("TCP: Failed to accept connection: %s\n", strerror(errno));
            }
            continue;
        }
        
        printf("TCP: New client connected from %s\n", inet_ntoa(client_addr.sin_addr));
        
        // Find available client slot
        pthread_mutex_lock(&transport->clients_mutex);
        int client_index = -1;
        for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
            if (!transport->clients[i].active) {
                client_index = i;
                transport->clients[i].socket = client_socket;
                transport->clients[i].index = i;
                transport->clients[i].transport = transport;
                transport->clients[i].active = true;
                transport->client_count++;
                break;
            }
        }
        pthread_mutex_unlock(&transport->clients_mutex);
        
        if (client_index >= 0) {
            // Create client handler thread
            int result = pthread_create(&transport->clients[client_index].thread, NULL, 
                                      tcp_client_handler, &transport->clients[client_index]);
            if (result != 0) {
                printf("TCP: Failed to create client thread: %s\n", strerror(result));
                close(client_socket);
                pthread_mutex_lock(&transport->clients_mutex);
                transport->clients[client_index].active = false;
                transport->client_count--;
                pthread_mutex_unlock(&transport->clients_mutex);
            } else {
                pthread_detach(transport->clients[client_index].thread);
            }
        } else {
            printf("TCP: Maximum clients reached (%d), dropping connection\n", MAX_TCP_CLIENTS);
            close(client_socket);
        }
    }
    
    close(transport->server_socket);
    transport->server_socket = -1;
    return NULL;
}

// TCP client handler thread
void* tcp_client_handler(void* arg) {
    tcp_client_data_t* client_data = (tcp_client_data_t*)arg;
    int client_socket = client_data->socket;
    int client_index = client_data->index;
    tcp_transport_t* transport = client_data->transport;
    
    char buffer[TCP_BUFFER_SIZE];
    char response_buffer[TCP_RESPONSE_SIZE];
    
    printf("TCP: Client handler started for index %d\n", client_index);
    
    while (transport->running && client_data->active) {
        // Read request from client
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received < 0) {
                printf("TCP: Client %d recv error: %s\n", client_index, strerror(errno));
            } else {
                printf("TCP: Client %d disconnected\n", client_index);
            }
            break;
        }
        
        buffer[bytes_received] = '\0';
        printf("TCP: Client %d sent %zd bytes: %.100s%s\n", 
               client_index, bytes_received, buffer, 
               bytes_received > 100 ? "..." : "");
        
        // Note: Direct string processing - would need proper MCP request/response conversion
        // For now, just echo back a basic success response
        snprintf(response_buffer, sizeof(response_buffer), 
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"status\":\"received\",\"bytes\":%zd}}", 
                bytes_received);
        size_t response_size = strlen(response_buffer);
        int result = 0; // Success
        
        if (result == 0 && response_size > 0) {
            // Send response back to client
            ssize_t bytes_sent = send(client_socket, response_buffer, response_size, 0);
            if (bytes_sent < 0) {
                printf("TCP: Failed to send response to client %d: %s\n", client_index, strerror(errno));
                break;
            }
            printf("TCP: Sent %zd bytes to client %d\n", bytes_sent, client_index);
        } else {
            // Send error response
            const char* error_response = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32603,\"message\":\"Internal error\"},\"id\":null}\\n";
            ssize_t bytes_sent = send(client_socket, error_response, strlen(error_response), 0);
            if (bytes_sent < 0) {
                printf("TCP: Failed to send error response to client %d: %s\n", client_index, strerror(errno));
                break;
            }
        }
    }
    
    // Cleanup client connection
    printf("TCP: Closing client %d connection\n", client_index);
    close(client_socket);
    
    pthread_mutex_lock(&transport->clients_mutex);
    transport->clients[client_index].active = false;
    transport->clients[client_index].socket = -1;
    transport->client_count--;
    pthread_mutex_unlock(&transport->clients_mutex);
    
    return NULL;
}

// Start TCP server
int tcp_server_start(tcp_transport_t* transport) {
    if (!transport || transport->running) {
        return -1;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&transport->clients_mutex, NULL) != 0) {
        printf("TCP: Failed to initialize clients mutex: %s\n", strerror(errno));
        return -1;
    }
    
    transport->running = true;
    transport->client_count = 0;
    
    // Initialize all client slots
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        transport->clients[i].socket = -1;
        transport->clients[i].index = i;
        transport->clients[i].transport = transport;
        transport->clients[i].active = false;
    }
    
    // Create server thread
    int result = pthread_create(&transport->server_thread, NULL, tcp_server_thread, transport);
    if (result != 0) {
        printf("TCP: Failed to create server thread: %s\n", strerror(result));
        transport->running = false;
        pthread_mutex_destroy(&transport->clients_mutex);
        return -1;
    }
    
    // Give server thread time to bind to port
    usleep(100000); // 100ms
    
    transport->connected = true;
    return 0;
}

// Stop TCP server
int tcp_server_stop(tcp_transport_t* transport) {
    if (!transport || !transport->running) {
        return 0;
    }
    
    printf("TCP: Stopping server...\n");
    transport->running = false;
    
    // Close server socket to unblock accept()
    if (transport->server_socket >= 0) {
        close(transport->server_socket);
        transport->server_socket = -1;
    }
    
    // Wait for server thread
    pthread_join(transport->server_thread, NULL);
    
    // Close all client connections
    pthread_mutex_lock(&transport->clients_mutex);
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (transport->clients[i].active && transport->clients[i].socket >= 0) {
            close(transport->clients[i].socket);
            transport->clients[i].active = false;
            transport->clients[i].socket = -1;
        }
    }
    transport->client_count = 0;
    pthread_mutex_unlock(&transport->clients_mutex);
    
    pthread_mutex_destroy(&transport->clients_mutex);
    transport->connected = false;
    
    printf("TCP: Server stopped\n");
    return 0;
}

// Send data to all connected clients
int tcp_send_to_all_clients(tcp_transport_t* transport, const char* data, size_t len) {
    if (!transport || !data || len == 0) {
        return -1;
    }
    
    int clients_sent = 0;
    
    pthread_mutex_lock(&transport->clients_mutex);
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (transport->clients[i].active && transport->clients[i].socket >= 0) {
            ssize_t bytes_sent = send(transport->clients[i].socket, data, len, 0);
            if (bytes_sent > 0) {
                clients_sent++;
            } else {
                printf("TCP: Failed to send to client %d: %s\n", i, strerror(errno));
            }
        }
    }
    pthread_mutex_unlock(&transport->clients_mutex);
    
    return clients_sent;
}

// Streaming support - send chunk to all clients
int tcp_send_stream_chunk(transport_base_t* base, const mcp_stream_chunk_t* chunk) {
    tcp_transport_t* transport = (tcp_transport_t*)base;
    if (!transport || !chunk) {
        return -1;
    }
    
    // Format chunk as JSON-RPC response
    char chunk_json[4096];
    int result = mcp_adapter_format_stream_chunk(chunk, chunk_json, sizeof(chunk_json));
    if (result != MCP_ADAPTER_OK) {
        printf("TCP: Failed to format stream chunk\n");
        return -1;
    }
    
    // Send to all connected clients
    int clients_sent = tcp_send_to_all_clients(transport, chunk_json, strlen(chunk_json));
    printf("TCP: Sent stream chunk to %d clients\n", clients_sent);
    
    return clients_sent > 0 ? 0 : -1;
}

// Get connection count
int tcp_get_connection_count(transport_base_t* base) {
    tcp_transport_t* transport = (tcp_transport_t*)base;
    if (!transport) {
        return -1;
    }
    
    pthread_mutex_lock(&transport->clients_mutex);
    size_t count = transport->client_count;
    pthread_mutex_unlock(&transport->clients_mutex);
    
    return (int)count;
}

// Test connection drop simulation
bool tcp_test_connection_drop(void) {
    tcp_transport_t* transport = &g_tcp_transport;
    
    if (!transport->running || transport->client_count == 0) {
        return false; // No connections to drop
    }
    
    // Find first active client and close its connection
    pthread_mutex_lock(&transport->clients_mutex);
    for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
        if (transport->clients[i].active && transport->clients[i].socket >= 0) {
            printf("TCP: Simulating connection drop for client %d\n", i);
            close(transport->clients[i].socket);
            transport->clients[i].socket = -1;
            transport->clients[i].active = false;
            transport->client_count--;
            pthread_mutex_unlock(&transport->clients_mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&transport->clients_mutex);
    
    return false;
}

// Create new TCP transport instance
tcp_transport_t* tcp_transport_create(void) {
    tcp_transport_t* transport = malloc(sizeof(tcp_transport_t));
    if (!transport) {
        return NULL;
    }
    
    memset(transport, 0, sizeof(tcp_transport_t));
    
    // Initialize base transport interface
    strncpy(transport->base.name, "tcp", sizeof(transport->base.name) - 1);
    transport->base.name[sizeof(transport->base.name) - 1] = '\0';
    transport->base.state = TRANSPORT_STATE_DISCONNECTED;
    transport->base.initialized = false;
    transport->base.running = false;
    transport->base.init = tcp_transport_base_init;
    transport->base.shutdown = tcp_transport_base_shutdown;
    transport->base.connect = tcp_transport_base_connect;
    transport->base.disconnect = tcp_transport_base_disconnect;
    transport->base.send = tcp_transport_base_send;
    transport->base.recv = tcp_transport_base_recv;
    transport->base.is_connected = tcp_transport_base_is_connected;
    transport->base.send_stream_chunk = (int (*)(struct transport_base*, const void*))tcp_send_stream_chunk;
    transport->base.get_connection_count = tcp_get_connection_count;
    transport->base.impl_data = transport;
    
    transport->server_socket = -1;
    
    return transport;
}

// Legacy functions for backward compatibility
int tcp_transport_init(void* config) {
    if (!config) return -1;
    
    tcp_transport_config_t* cfg = (tcp_transport_config_t*)config;
    g_config = *cfg;
    
    // Copy host string
    if (cfg->host) {
        g_config.host = strdup(cfg->host);
    }
    
    g_config.connected = false;
    g_config.socket_fd = -1;
    g_config.initialized = true;
    g_config.running = true;
    
    // Initialize new transport
    g_tcp_transport.host = g_config.host ? strdup(g_config.host) : NULL;
    g_tcp_transport.port = g_config.port;
    g_tcp_transport.initialized = true;
    
    return 0;
}

int tcp_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0) {
        return -1;
    }
    
    // Use new multi-client implementation if server mode
    if (g_config.is_server && g_tcp_transport.running) {
        return tcp_send_to_all_clients(&g_tcp_transport, data, len) > 0 ? 0 : -1;
    }
    
    // Legacy client mode implementation
    if (!g_config.connected) {
        return -1;
    }
    
    int target_fd = g_config.is_server ? g_client_fd : g_config.socket_fd;
    if (target_fd < 0) {
        return -1;
    }
    
    ssize_t sent = send(target_fd, data, len, 0);
    if (sent == (ssize_t)len) {
        g_response_sent = true;
        return 0;
    }
    return -1;
}

int tcp_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_config.initialized || !buffer || buffer_size == 0) {
        return -1;
    }
    
    // Use legacy implementation for now - this will be called less frequently
    // with the new multi-client server handling requests in separate threads
    if (!g_config.connected) {
        return -1;
    }
    
    if (g_config.is_server) {
        // Server mode: accept new connections if needed
        if (g_client_fd < 0) {
            fd_set readfds;
            struct timeval timeout;
            
            FD_ZERO(&readfds);
            FD_SET(g_config.socket_fd, &readfds);
            
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_usec = (timeout_ms % 1000) * 1000;
            
            int result = select(g_config.socket_fd + 1, &readfds, NULL, NULL, &timeout);
            if (result <= 0) {
                return -1;
            }
            
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            g_client_fd = accept(g_config.socket_fd, (struct sockaddr*)&client_addr, &client_len);
            if (g_client_fd < 0) {
                return -1;
            }
            g_client_has_data = true;
        }
        
        if (g_response_sent) {
            fd_set readfds;
            struct timeval short_timeout = {0, 1000};
            
            FD_ZERO(&readfds);
            FD_SET(g_client_fd, &readfds);
            
            int result = select(g_client_fd + 1, &readfds, NULL, NULL, &short_timeout);
            if (result > 0) {
                char test_buffer[1];
                ssize_t test_recv = recv(g_client_fd, test_buffer, 1, MSG_PEEK);
                if (test_recv == 0) {
                    close(g_client_fd);
                    g_client_fd = -1;
                    g_client_has_data = false;
                    g_response_sent = false;
                    return -1;
                }
            }
            g_response_sent = false;
            return -1;
        }
        
        if (!g_client_has_data) {
            fd_set readfds;
            struct timeval timeout;
            
            FD_ZERO(&readfds);
            FD_SET(g_client_fd, &readfds);
            
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_usec = (timeout_ms % 1000) * 1000;
            
            int result = select(g_client_fd + 1, &readfds, NULL, NULL, &timeout);
            if (result <= 0) {
                return -1;
            }
        }
        
        ssize_t received = recv(g_client_fd, buffer, buffer_size - 1, 0);
        if (received < 0) {
            close(g_client_fd);
            g_client_fd = -1;
            g_client_has_data = false;
            g_response_sent = false;
            return -1;
        } else if (received == 0) {
            close(g_client_fd);
            g_client_fd = -1;
            g_client_has_data = false;
            g_response_sent = false;
            return -1;
        }
        
        g_client_has_data = false;
        buffer[received] = '\0';
        return 0;
    } else {
        // Client mode implementation unchanged
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(g_config.socket_fd, &readfds);
        
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        
        int result = select(g_config.socket_fd + 1, &readfds, NULL, NULL, &timeout);
        if (result <= 0) {
            return -1;
        }
        
        ssize_t received = recv(g_config.socket_fd, buffer, buffer_size - 1, 0);
        if (received <= 0) {
            return -1;
        }
        
        buffer[received] = '\0';
        return 0;
    }
}

int tcp_transport_connect(void) {
    if (!g_config.initialized) return -1;
    
    if (g_config.connected) return 0;
    
    // Create socket
    g_config.socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_config.socket_fd < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_config.port);
    
    if (g_config.is_server) {
        // Initialize new multi-client server
        g_tcp_transport.host = g_config.host ? strdup(g_config.host) : NULL;
        g_tcp_transport.port = g_config.port;
        
        // Start multi-client server
        if (tcp_server_start(&g_tcp_transport) == 0) {
            printf("✅ TCP multi-client server started on port %d\n", g_config.port);
        }
        
        // Legacy server setup for backward compatibility
        addr.sin_addr.s_addr = INADDR_ANY;
        
        int reuse = 1;
        setsockopt(g_config.socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        
        if (bind(g_config.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            printf("TCP bind failed on port %d: %s\n", g_config.port, strerror(errno));
            close(g_config.socket_fd);
            return -1;
        }
        
        if (listen(g_config.socket_fd, 5) < 0) {
            printf("TCP listen failed on port %d: %s\n", g_config.port, strerror(errno));
            close(g_config.socket_fd);
            return -1;
        }
        
        printf("✅ TCP legacy server listening on port %d\n", g_config.port);
    } else {
        // Client mode
        if (inet_pton(AF_INET, g_config.host, &addr.sin_addr) <= 0) {
            close(g_config.socket_fd);
            return -1;
        }
        
        if (connect(g_config.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(g_config.socket_fd);
            return -1;
        }
    }
    
    g_config.connected = true;
    return 0;
}

int tcp_transport_disconnect(void) {
    if (!g_config.initialized) return -1;
    
    // Stop multi-client server
    tcp_server_stop(&g_tcp_transport);
    
    // Close client connection if exists
    if (g_client_fd >= 0) {
        close(g_client_fd);
        g_client_fd = -1;
        g_client_has_data = false;
        g_response_sent = false;
    }
    
    if (g_config.socket_fd >= 0) {
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
    }
    
    g_config.connected = false;
    return 0;
}

bool tcp_transport_is_connected(void) {
    return g_config.initialized && (g_config.connected || g_tcp_transport.connected);
}

void tcp_transport_shutdown(void) {
    tcp_transport_disconnect();
    
    if (g_config.host) {
        free(g_config.host);
        g_config.host = NULL;
    }
    
    if (g_tcp_transport.host) {
        free(g_tcp_transport.host);
        g_tcp_transport.host = NULL;
    }
    
    g_config.running = false;
    g_config.initialized = false;
    g_tcp_transport.initialized = false;
}

// Transport base interface implementations
static int tcp_transport_base_init(transport_base_t* base, void* config) {
    return tcp_transport_init(config);
}

static void tcp_transport_base_shutdown(transport_base_t* base) {
    tcp_transport_shutdown();
}

static int tcp_transport_base_connect(transport_base_t* base) {
    return tcp_transport_connect();
}

static int tcp_transport_base_disconnect(transport_base_t* base) {
    return tcp_transport_disconnect();
}

static int tcp_transport_base_send(transport_base_t* base, const char* data, size_t len) {
    return tcp_transport_send_raw(data, len);
}

static int tcp_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms) {
    return tcp_transport_recv_raw(buffer, buffer_size, timeout_ms);
}

static int tcp_transport_base_is_connected(transport_base_t* base) {
    return tcp_transport_is_connected() ? 1 : 0;
}

static transport_base_t g_tcp_transport_base = {
    .name = "tcp",
    .state = TRANSPORT_STATE_DISCONNECTED,
    .initialized = false,
    .running = false,
    .init = tcp_transport_base_init,
    .shutdown = tcp_transport_base_shutdown,
    .connect = tcp_transport_base_connect,
    .disconnect = tcp_transport_base_disconnect,
    .send = tcp_transport_base_send,
    .recv = tcp_transport_base_recv,
    .is_connected = tcp_transport_base_is_connected,
    .send_stream_chunk = (int (*)(struct transport_base*, const void*))tcp_send_stream_chunk,
    .get_connection_count = tcp_get_connection_count,
    .impl_data = &g_tcp_transport,
    .on_error = NULL,
    .on_state_change = NULL
};

transport_base_t* tcp_transport_get_interface(void) {
    return &g_tcp_transport_base;
}