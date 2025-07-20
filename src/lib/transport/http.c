#include "http.h"
#include "common/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static http_transport_config_t g_config = {0};
static int g_socket_fd = -1;

int http_transport_init(void* config) {
    if (!config) return -1;
    
    http_transport_config_t* cfg = (http_transport_config_t*)config;
    g_config = *cfg;
    
    // Copy string fields
    if (cfg->host) {
        g_config.host = strdup(cfg->host);
    }
    if (cfg->path) {
        g_config.path = strdup(cfg->path);
    }
    
    // Set defaults
    g_config.timeout_ms = cfg->timeout_ms ? cfg->timeout_ms : 30000;
    g_config.keep_alive = cfg->keep_alive;
    g_config.connected = false;
    
    g_config.initialized = true;
    g_config.running = true;
    return 0;
}

int http_transport_connect(void) {
    if (!g_config.initialized) return -1;
    
    if (g_config.connected) return 0; // Already connected
    
    // Create socket
    g_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_socket_fd < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_config.port);
    
    if (inet_pton(AF_INET, g_config.host, &addr.sin_addr) <= 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
        return -1;
    }
    
    if (connect(g_socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
        return -1;
    }
    
    g_config.connected = true;
    return 0;
}

int http_transport_disconnect(void) {
    if (!g_config.initialized) return -1;
    
    if (g_socket_fd >= 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
    }
    
    g_config.connected = false;
    return 0;
}

bool http_transport_is_connected(void) {
    return g_config.initialized && g_config.connected && g_socket_fd >= 0;
}

int http_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0) {
        return -1;
    }
    
    // For HTTP, we need to wrap the data in an HTTP request
    char http_request[8192];
    int request_len = snprintf(http_request, sizeof(http_request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: %s\r\n"
        "\r\n"
        "%.*s",
        g_config.path ? g_config.path : "/",
        g_config.host,
        g_config.port,
        len,
        g_config.keep_alive ? "keep-alive" : "close",
        (int)len, data);
    
    if (request_len >= (int)sizeof(http_request)) {
        return -1; // Request too large
    }
    
    // Connect if not already connected
    if (!g_config.connected) {
        if (http_transport_connect() != 0) {
            return -1;
        }
    }
    
    // Send HTTP request
    ssize_t sent = send(g_socket_fd, http_request, request_len, 0);
    
    // If not keeping alive, disconnect after sending
    if (!g_config.keep_alive) {
        http_transport_disconnect();
    }
    
    return (sent == request_len) ? 0 : -1;
}

int http_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_config.initialized || !buffer || buffer_size == 0) {
        return -1;
    }
    
    // For HTTP, this would typically be used to read the response
    // after sending a request
    if (!g_config.connected) {
        return -1;
    }
    
    // Use select for timeout
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(g_socket_fd, &readfds);
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(g_socket_fd + 1, &readfds, NULL, NULL, &timeout);
    if (result <= 0) {
        return -1; // Timeout or error
    }
    
    // Read HTTP response
    char http_response[8192];
    ssize_t received = recv(g_socket_fd, http_response, sizeof(http_response) - 1, 0);
    if (received <= 0) {
        return -1;
    }
    
    http_response[received] = '\0';
    
    // Extract body from HTTP response
    return http_transport_parse_http_response(http_response, buffer, buffer_size);
}

int http_transport_send_http_request(const char* method, const char* path, const char* body, char* response, size_t response_size) {
    if (!method || !path || !response || response_size == 0) {
        return -1;
    }
    
    // Connect if needed
    if (!g_config.connected) {
        if (http_transport_connect() != 0) {
            return -1;
        }
    }
    
    // Build HTTP request
    char request[8192];
    int request_len;
    
    if (body) {
        request_len = snprintf(request, sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n"
            "Connection: %s\r\n"
            "\r\n"
            "%s",
            method, path, g_config.host, g_config.port, strlen(body),
            g_config.keep_alive ? "keep-alive" : "close", body);
    } else {
        request_len = snprintf(request, sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Connection: %s\r\n"
            "\r\n",
            method, path, g_config.host, g_config.port,
            g_config.keep_alive ? "keep-alive" : "close");
    }
    
    // Send request
    ssize_t sent = send(g_socket_fd, request, request_len, 0);
    if (sent != request_len) {
        return -1;
    }
    
    // Read response
    ssize_t received = recv(g_socket_fd, response, response_size - 1, 0);
    if (received <= 0) {
        return -1;
    }
    
    response[received] = '\0';
    
    // Disconnect if not keeping alive
    if (!g_config.keep_alive) {
        http_transport_disconnect();
    }
    
    return 0;
}

int http_transport_parse_http_response(const char* response, char* body, size_t body_size) {
    if (!response || !body || body_size == 0) {
        return -1;
    }
    
    // Find the end of headers (double CRLF)
    const char* body_start = strstr(response, "\r\n\r\n");
    if (!body_start) {
        return -1;
    }
    
    body_start += 4; // Skip the "\r\n\r\n"
    
    // Copy body
    size_t body_len = strlen(body_start);
    if (body_len >= body_size) {
        body_len = body_size - 1;
    }
    
    memcpy(body, body_start, body_len);
    body[body_len] = '\0';
    
    return 0;
}

void http_transport_shutdown(void) {
    http_transport_disconnect();
    
    if (g_config.host) {
        free(g_config.host);
        g_config.host = NULL;
    }
    
    if (g_config.path) {
        free(g_config.path);
        g_config.path = NULL;
    }
    
    g_config.running = false;
    g_config.initialized = false;
}

// Forward declarations for transport_base interface
static int http_transport_base_init(transport_base_t* base, void* config);
static void http_transport_base_shutdown(transport_base_t* base);
static int http_transport_base_connect(transport_base_t* base);
static int http_transport_base_disconnect(transport_base_t* base);
static int http_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int http_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int http_transport_base_is_connected(transport_base_t* base);

static transport_base_t g_http_transport = {
    .name = "http",
    .state = TRANSPORT_STATE_DISCONNECTED,
    .initialized = false,
    .running = false,
    .init = http_transport_base_init,
    .shutdown = http_transport_base_shutdown,
    .connect = http_transport_base_connect,
    .disconnect = http_transport_base_disconnect,
    .send = http_transport_base_send,
    .recv = http_transport_base_recv,
    .is_connected = http_transport_base_is_connected,
    .impl_data = NULL,
    .on_error = NULL,
    .on_state_change = NULL
};

// Transport base interface implementations
static int http_transport_base_init(transport_base_t* base, void* config) {
    return http_transport_init(config);
}

static void http_transport_base_shutdown(transport_base_t* base) {
    http_transport_shutdown();
}

static int http_transport_base_connect(transport_base_t* base) {
    return http_transport_connect();
}

static int http_transport_base_disconnect(transport_base_t* base) {
    return http_transport_disconnect();
}

static int http_transport_base_send(transport_base_t* base, const char* data, size_t len) {
    return http_transport_send_raw(data, len);
}

static int http_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms) {
    return http_transport_recv_raw(buffer, buffer_size, timeout_ms);
}

static int http_transport_base_is_connected(transport_base_t* base) {
    return http_transport_is_connected() ? 1 : 0;
}

transport_base_t* http_transport_get_interface(void) {
    return &g_http_transport;
}