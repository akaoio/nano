#define _DEFAULT_SOURCE
#include "udp.h"
#include "common/types.h"
#include "mcp/transport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>

static udp_transport_config_t g_config = {0};
static struct sockaddr_in g_last_sender = {0};  // Store last sender address

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
    return 0;
}

int udp_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0 || !g_config.connected) {
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
    if (!g_config.initialized || !buffer || buffer_size == 0 || !g_config.connected) {
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
        close(g_config.socket_fd);
        return -1;
    }
    
    g_config.connected = true;
    return 0;
}

int udp_transport_disconnect(void) {
    if (!g_config.initialized) return -1;
    
    if (g_config.socket_fd >= 0) {
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
    }
    
    g_config.connected = false;
    return 0;
}

bool udp_transport_is_connected(void) {
    return g_config.initialized && g_config.connected && g_config.socket_fd >= 0;
}

void udp_transport_shutdown(void) {
    udp_transport_disconnect();
    
    if (g_config.host) {
        free(g_config.host);
        g_config.host = NULL;
    }
    
    g_config.running = false;
    g_config.initialized = false;
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

// Forward declarations for transport_base interface
static int udp_transport_base_init(transport_base_t* base, void* config);
static void udp_transport_base_shutdown(transport_base_t* base);
static int udp_transport_base_connect(transport_base_t* base);
static int udp_transport_base_disconnect(transport_base_t* base);
static int udp_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int udp_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int udp_transport_base_is_connected(transport_base_t* base);

static transport_base_t g_udp_transport = {
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
    .impl_data = NULL,
    .on_error = NULL,
    .on_state_change = NULL
};

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

transport_base_t* udp_transport_get_interface(void) {
    return &g_udp_transport;
}
