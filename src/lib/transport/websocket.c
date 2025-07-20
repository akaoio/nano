#include "websocket.h"
#include "common/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

static ws_transport_config_t g_config = {0};

int ws_transport_init(void* config) {
    if (!config) return -1;
    
    ws_transport_config_t* cfg = (ws_transport_config_t*)config;
    g_config = *cfg;
    
    // Copy string fields
    if (cfg->host) {
        g_config.host = strdup(cfg->host);
    }
    if (cfg->path) {
        g_config.path = strdup(cfg->path);
    }
    
    // WebSocket defaults
    g_config.mask_frames = true; // Client should mask frames
    g_config.connected = false;
    g_config.socket_fd = -1;
    
    // Generate random WebSocket key for handshake
    srand(time(NULL));
    for (int i = 0; i < 16; i++) {
        sprintf(g_config.sec_key + i * 2, "%02x", rand() % 256);
    }
    
    g_config.initialized = true;
    g_config.running = true;
    return 0;
}

int ws_transport_connect(void) {
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
    
    if (inet_pton(AF_INET, g_config.host, &addr.sin_addr) <= 0) {
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
        return -1;
    }
    
    if (connect(g_config.socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
        return -1;
    }
    
    // Perform WebSocket handshake
    if (ws_transport_perform_handshake() != 0) {
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
        return -1;
    }
    
    g_config.connected = true;
    return 0;
}

int ws_transport_disconnect(void) {
    if (!g_config.initialized) return -1;
    
    if (g_config.socket_fd >= 0) {
        // Send close frame
        char close_frame[] = {0x88, 0x00}; // Close frame with no payload
        send(g_config.socket_fd, close_frame, sizeof(close_frame), 0);
        
        close(g_config.socket_fd);
        g_config.socket_fd = -1;
    }
    
    g_config.connected = false;
    return 0;
}

bool ws_transport_is_connected(void) {
    return g_config.initialized && g_config.connected && g_config.socket_fd >= 0;
}

int ws_transport_perform_handshake(void) {
    if (g_config.socket_fd < 0) return -1;
    
    // Build WebSocket handshake request
    char handshake[1024];
    int handshake_len = snprintf(handshake, sizeof(handshake),
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        g_config.path ? g_config.path : "/",
        g_config.host,
        g_config.port,
        g_config.sec_key);
    
    // Send handshake
    ssize_t sent = send(g_config.socket_fd, handshake, handshake_len, 0);
    if (sent != handshake_len) {
        return -1;
    }
    
    // Read handshake response
    char response[1024];
    ssize_t received = recv(g_config.socket_fd, response, sizeof(response) - 1, 0);
    if (received <= 0) {
        return -1;
    }
    
    response[received] = '\0';
    
    // Simple validation - check for "101 Switching Protocols"
    if (strstr(response, "101 Switching Protocols") == NULL) {
        return -1;
    }
    
    return 0;
}

int ws_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0 || !g_config.connected) {
        return -1;
    }
    
    return ws_transport_send_ws_frame(data, len, true);
}

int ws_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms) {
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
    
    return ws_transport_recv_ws_frame(buffer, buffer_size);
}

int ws_transport_send_ws_frame(const char* data, size_t len, bool is_text) {
    if (!data || len == 0 || g_config.socket_fd < 0) {
        return -1;
    }
    
    // Build WebSocket frame
    char frame[8192];
    size_t frame_len = 0;
    
    // First byte: FIN bit + opcode
    frame[0] = 0x80 | (is_text ? 0x01 : 0x02); // FIN + TEXT or BINARY
    frame_len++;
    
    // Payload length and masking
    if (len < 126) {
        frame[1] = (char)(len | (g_config.mask_frames ? 0x80 : 0x00));
        frame_len++;
    } else if (len < 65536) {
        frame[1] = 126 | (g_config.mask_frames ? 0x80 : 0x00);
        frame[2] = (len >> 8) & 0xFF;
        frame[3] = len & 0xFF;
        frame_len += 3;
    } else {
        // Extended payload length (64-bit) - simplified for 32-bit
        frame[1] = 127 | (g_config.mask_frames ? 0x80 : 0x00);
        frame[2] = frame[3] = frame[4] = frame[5] = 0; // High 32 bits
        frame[6] = (len >> 24) & 0xFF;
        frame[7] = (len >> 16) & 0xFF;
        frame[8] = (len >> 8) & 0xFF;
        frame[9] = len & 0xFF;
        frame_len += 9;
    }
    
    // Add masking key and mask payload if client
    if (g_config.mask_frames) {
        uint32_t mask = rand();
        memcpy(frame + frame_len, &mask, 4);
        frame_len += 4;
        
        // Mask the payload
        for (size_t i = 0; i < len; i++) {
            frame[frame_len + i] = data[i] ^ ((char*)&mask)[i % 4];
        }
    } else {
        // Copy payload without masking
        memcpy(frame + frame_len, data, len);
    }
    frame_len += len;
    
    // Send frame
    ssize_t sent = send(g_config.socket_fd, frame, frame_len, 0);
    return (sent == (ssize_t)frame_len) ? 0 : -1;
}

int ws_transport_recv_ws_frame(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0 || g_config.socket_fd < 0) {
        return -1;
    }
    
    // Read frame header (minimum 2 bytes)
    char header[14]; // Max header size
    ssize_t received = recv(g_config.socket_fd, header, 2, 0);
    if (received < 2) {
        return -1;
    }
    
    bool fin = (header[0] & 0x80) != 0;
    uint8_t opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;
    
    size_t header_size = 2;
    
    // Read extended payload length if needed
    if (payload_len == 126) {
        received = recv(g_config.socket_fd, header + 2, 2, 0);
        if (received < 2) return -1;
        payload_len = (header[2] << 8) | header[3];
        header_size += 2;
    } else if (payload_len == 127) {
        received = recv(g_config.socket_fd, header + 2, 8, 0);
        if (received < 8) return -1;
        // For simplicity, only use lower 32 bits
        payload_len = (header[6] << 24) | (header[7] << 16) | (header[8] << 8) | header[9];
        header_size += 8;
    }
    
    // Read masking key if present
    char mask[4] = {0};
    if (masked) {
        received = recv(g_config.socket_fd, mask, 4, 0);
        if (received < 4) return -1;
        header_size += 4;
    }
    
    // Read payload
    if (payload_len >= buffer_size) {
        return -1; // Payload too large
    }
    
    received = recv(g_config.socket_fd, buffer, payload_len, 0);
    if (received != (ssize_t)payload_len) {
        return -1;
    }
    
    // Unmask payload if needed
    if (masked) {
        for (uint64_t i = 0; i < payload_len; i++) {
            buffer[i] ^= mask[i % 4];
        }
    }
    
    buffer[payload_len] = '\0';
    
    // Handle different frame types
    if (opcode == 0x08) { // Close frame
        g_config.connected = false;
        return -1;
    } else if (opcode == 0x09) { // Ping frame
        // Send pong response
        ws_transport_send_ws_frame(buffer, payload_len, false);
        return ws_transport_recv_ws_frame(buffer, buffer_size); // Read next frame
    } else if (opcode == 0x0A) { // Pong frame
        return ws_transport_recv_ws_frame(buffer, buffer_size); // Read next frame
    }
    
    return 0;
}

void ws_transport_shutdown(void) {
    ws_transport_disconnect();
    
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
static int ws_transport_base_init(transport_base_t* base, void* config);
static void ws_transport_base_shutdown(transport_base_t* base);
static int ws_transport_base_connect(transport_base_t* base);
static int ws_transport_base_disconnect(transport_base_t* base);
static int ws_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int ws_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int ws_transport_base_is_connected(transport_base_t* base);

static transport_base_t g_ws_transport = {
    .name = "websocket",
    .state = TRANSPORT_STATE_DISCONNECTED,
    .initialized = false,
    .running = false,
    .init = ws_transport_base_init,
    .shutdown = ws_transport_base_shutdown,
    .connect = ws_transport_base_connect,
    .disconnect = ws_transport_base_disconnect,
    .send = ws_transport_base_send,
    .recv = ws_transport_base_recv,
    .is_connected = ws_transport_base_is_connected,
    .impl_data = NULL,
    .on_error = NULL,
    .on_state_change = NULL
};

// Transport base interface implementations
static int ws_transport_base_init(transport_base_t* base, void* config) {
    return ws_transport_init(config);
}

static void ws_transport_base_shutdown(transport_base_t* base) {
    ws_transport_shutdown();
}

static int ws_transport_base_connect(transport_base_t* base) {
    return ws_transport_connect();
}

static int ws_transport_base_disconnect(transport_base_t* base) {
    return ws_transport_disconnect();
}

static int ws_transport_base_send(transport_base_t* base, const char* data, size_t len) {
    return ws_transport_send_raw(data, len);
}

static int ws_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms) {
    return ws_transport_recv_raw(buffer, buffer_size, timeout_ms);
}

static int ws_transport_base_is_connected(transport_base_t* base) {
    return ws_transport_is_connected() ? 1 : 0;
}

transport_base_t* ws_transport_get_interface(void) {
    return &g_ws_transport;
}