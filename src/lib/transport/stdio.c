#include "stdio.h"
#include "common/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

static stdio_transport_config_t g_config = {0};
static bool g_connected = false;

int stdio_transport_init(void* config) {
    if (config) {
        stdio_transport_config_t* cfg = (stdio_transport_config_t*)config;
        g_config = *cfg;
    }
    
    // Set line buffering for stdout for real-time communication
    if (g_config.line_buffered) {
        setlinebuf(stdout);
    }
    
    g_config.initialized = true;
    g_config.running = true;
    
    // Log initialization to stderr
    if (g_config.log_to_stderr) {
        stdio_transport_log_to_stderr("STDIO transport initialized");
    }
    
    return 0;
}

int stdio_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0) return -1;
    
    // Pure data transmission - write exactly what we're given
    size_t written = fwrite(data, 1, len, stdout);
    
    // Force immediate output for real-time communication
    fflush(stdout);
    
    return (written == len) ? 0 : -1;
}

int stdio_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_config.initialized || !buffer || buffer_size == 0) return -1;
    
    // Use select to wait for input with timeout
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
    if (result <= 0) {
        return -1; // Timeout or error
    }
    
    // Read raw data from stdin
    if (!fgets(buffer, buffer_size, stdin)) {
        return -1;
    }
    
    return 0;
}

void stdio_transport_shutdown(void) {
    if (g_config.log_to_stderr) {
        stdio_transport_log_to_stderr("STDIO transport shutdown");
    }
    
    g_connected = false;
    g_config.running = false;
    g_config.initialized = false;
}

int stdio_transport_connect(void) {
    if (!g_config.initialized) return -1;
    g_connected = true; // STDIO is always "connected"
    return 0;
}

int stdio_transport_disconnect(void) {
    if (!g_config.initialized) return -1;
    g_connected = false;
    return 0;
}

bool stdio_transport_is_connected(void) {
    return g_config.initialized && g_connected;
}

void stdio_transport_log_to_stderr(const char* message) {
    if (!message || !g_config.log_to_stderr) return;
    
    fprintf(stderr, "[STDIO] %s\n", message);
    fflush(stderr);
}

// Forward declarations for transport_base interface
static int stdio_transport_base_init(transport_base_t* base, void* config);
static void stdio_transport_base_shutdown(transport_base_t* base);
static int stdio_transport_base_connect(transport_base_t* base);
static int stdio_transport_base_disconnect(transport_base_t* base);
static int stdio_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int stdio_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int stdio_transport_base_is_connected(transport_base_t* base);

static transport_base_t g_stdio_transport = {
    .name = "stdio",
    .state = TRANSPORT_STATE_DISCONNECTED,
    .initialized = false,
    .running = false,
    .init = stdio_transport_base_init,
    .shutdown = stdio_transport_base_shutdown,
    .connect = stdio_transport_base_connect,
    .disconnect = stdio_transport_base_disconnect,
    .send = stdio_transport_base_send,
    .recv = stdio_transport_base_recv,
    .is_connected = stdio_transport_base_is_connected,
    .impl_data = NULL,
    .on_error = NULL,
    .on_state_change = NULL
};

// Transport base interface implementations
static int stdio_transport_base_init(transport_base_t* base, void* config) {
    return stdio_transport_init(config);
}

static void stdio_transport_base_shutdown(transport_base_t* base) {
    stdio_transport_shutdown();
}

static int stdio_transport_base_connect(transport_base_t* base) {
    return stdio_transport_connect();
}

static int stdio_transport_base_disconnect(transport_base_t* base) {
    return stdio_transport_disconnect();
}

static int stdio_transport_base_send(transport_base_t* base, const char* data, size_t len) {
    return stdio_transport_send_raw(data, len);
}

static int stdio_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms) {
    return stdio_transport_recv_raw(buffer, buffer_size, timeout_ms);
}

static int stdio_transport_base_is_connected(transport_base_t* base) {
    return stdio_transport_is_connected() ? 1 : 0;
}


transport_base_t* stdio_transport_get_interface(void) {
    return &g_stdio_transport;
}
