#include "http_transport.h"
#include "../../../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static http_transport_config_t g_config = {0};

int http_transport_init(void* config) {
    if (!config) return -1;
    
    http_transport_config_t* cfg = (http_transport_config_t*)config;
    g_config.host = str_copy(cfg->host);
    g_config.port = cfg->port;
    g_config.path = str_copy(cfg->path ? cfg->path : "/");
    g_config.initialized = true;
    g_config.running = true;
    
    return 0;
}

int http_transport_send(const mcp_message_t* message) {
    if (!g_config.initialized || !message) return -1;
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    // Connect to server
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_config.port);
    inet_pton(AF_INET, g_config.host, &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    // Format JSON-RPC message
    char json_buffer[8192];
    if (mcp_format_json_rpc(message, json_buffer, sizeof(json_buffer)) != 0) {
        close(sock);
        return -1;
    }
    
    // Create HTTP POST request
    char http_request[16384];
    snprintf(http_request, sizeof(http_request),
            "POST %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            g_config.path, g_config.host, g_config.port, strlen(json_buffer), json_buffer);
    
    // Send request
    ssize_t sent = send(sock, http_request, strlen(http_request), 0);
    close(sock);
    
    return (sent > 0) ? 0 : -1;
}

int http_transport_recv(mcp_message_t* message, int timeout_ms) {
    // HTTP transport is typically request-response, not streaming
    // This would require implementing an HTTP server
    (void)message;
    (void)timeout_ms;
    return -1; // Not implemented for simple HTTP client
}

void http_transport_shutdown(void) {
    str_free(g_config.host);
    str_free(g_config.path);
    g_config.host = NULL;
    g_config.path = NULL;
    g_config.running = false;
    g_config.initialized = false;
}

static mcp_transport_t g_http_transport = {
    .init = http_transport_init,
    .send = http_transport_send,
    .recv = http_transport_recv,
    .shutdown = http_transport_shutdown
};

mcp_transport_t* http_transport_get_interface(void) {
    return &g_http_transport;
}
