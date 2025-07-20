#include "tcp.h"
#include "common/types.h"
#include "mcp/transport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

static tcp_transport_config_t g_config = {0};
static int g_client_fd = -1;  // Current client connection
static bool g_client_has_data = false;  // Track if client has pending data
static bool g_response_sent = false;  // Track if we just sent a response

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
    return 0;
}

int tcp_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0 || !g_config.connected) {
        return -1;
    }
    
    int target_fd = g_config.is_server ? g_client_fd : g_config.socket_fd;
    if (target_fd < 0) {
        return -1;  // No client connected in server mode
    }
    
    ssize_t sent = send(target_fd, data, len, 0);
    if (sent == (ssize_t)len) {
        g_response_sent = true;  // Mark that we sent a response
        return 0;
    }
    return -1;
}

int tcp_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_config.initialized || !buffer || buffer_size == 0 || !g_config.connected) {
        return -1;
    }
    
    if (g_config.is_server) {
        // Server mode: accept new connections if needed
        if (g_client_fd < 0) {
            // Use select to wait for incoming connections
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
            
            // Accept new client connection
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            g_client_fd = accept(g_config.socket_fd, (struct sockaddr*)&client_addr, &client_len);
            if (g_client_fd < 0) {
                return -1;
            }
            g_client_has_data = true;  // New connection, likely has data
        }
        
        // If we just sent a response, check if client has disconnected
        if (g_response_sent) {
            // Check if client is still connected with a short timeout
            fd_set readfds;
            struct timeval short_timeout = {0, 1000}; // 1ms
            
            FD_ZERO(&readfds);
            FD_SET(g_client_fd, &readfds);
            
            int result = select(g_client_fd + 1, &readfds, NULL, NULL, &short_timeout);
            if (result > 0) {
                // Data available, check if it's disconnection
                char test_buffer[1];
                ssize_t test_recv = recv(g_client_fd, test_buffer, 1, MSG_PEEK);
                if (test_recv == 0) {
                    // Client disconnected, reset for next connection
                    close(g_client_fd);
                    g_client_fd = -1;
                    g_client_has_data = false;
                    g_response_sent = false;
                    return -1;
                }
            }
            // If no immediate disconnection, don't try to read more data
            g_response_sent = false;
            return -1;
        }
        
        // Only try to read if we expect data or can check for data
        if (!g_client_has_data) {
            // Use the provided timeout to wait for data
            fd_set readfds;
            struct timeval timeout;
            
            FD_ZERO(&readfds);
            FD_SET(g_client_fd, &readfds);
            
            timeout.tv_sec = timeout_ms / 1000;
            timeout.tv_usec = (timeout_ms % 1000) * 1000;
            
            int result = select(g_client_fd + 1, &readfds, NULL, NULL, &timeout);
            if (result <= 0) {
                return -1; // No data available or timeout
            }
        }
        
        // Read data from client
        ssize_t received = recv(g_client_fd, buffer, buffer_size - 1, 0);
        if (received < 0) {
            // Error occurred, close and reset
            close(g_client_fd);
            g_client_fd = -1;
            g_client_has_data = false;
            g_response_sent = false;
            return -1;
        } else if (received == 0) {
            // Client cleanly closed connection, reset for next connection
            close(g_client_fd);
            g_client_fd = -1;
            g_client_has_data = false;
            g_response_sent = false;
            return -1;
        }
        
        g_client_has_data = false;  // Data consumed
        buffer[received] = '\0';
        
        // After processing this request and sending response,
        // the client will likely close the connection. Mark this
        // so we can reset the connection state properly.
        
        return 0;
    } else {
        // Client mode: read from server connection
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
        
        // Read data
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
    
    if (g_config.connected) return 0; // Already connected
    
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
        // Server mode - bind and listen (non-blocking)
        addr.sin_addr.s_addr = INADDR_ANY;
        
        int reuse = 1;
        setsockopt(g_config.socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        
        if (bind(g_config.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            printf("TCP bind failed on port %d: %s\n", g_config.port, strerror(errno));
            close(g_config.socket_fd);
            return -1;
        }
        
        if (listen(g_config.socket_fd, 5) < 0) {  // Allow multiple connections in queue
            printf("TCP listen failed on port %d: %s\n", g_config.port, strerror(errno));
            close(g_config.socket_fd);
            return -1;
        }
        
        printf("✅ TCP server listening on port %d\n", g_config.port);
        
        // Don't block on accept - just mark as ready to accept connections
        // The actual accept() will happen in recv or a separate thread
    } else {
        // Client mode - connect to server
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
    return g_config.initialized && g_config.connected && g_config.socket_fd >= 0;
}

void tcp_transport_shutdown(void) {
    tcp_transport_disconnect();
    
    if (g_config.host) {
        free(g_config.host);
        g_config.host = NULL;
    }
    
    g_config.running = false;
    g_config.initialized = false;
}

// Forward declarations for transport_base interface
static int tcp_transport_base_init(transport_base_t* base, void* config);
static void tcp_transport_base_shutdown(transport_base_t* base);
static int tcp_transport_base_connect(transport_base_t* base);
static int tcp_transport_base_disconnect(transport_base_t* base);
static int tcp_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int tcp_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int tcp_transport_base_is_connected(transport_base_t* base);

static transport_base_t g_tcp_transport = {
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
    .impl_data = NULL,
    .on_error = NULL,
    .on_state_change = NULL
};

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

transport_base_t* tcp_transport_get_interface(void) {
    return &g_tcp_transport;
}
