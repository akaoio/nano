#include "transport_simple.h"
#include "../../common/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

// STDIO Simple Transport - Pure stdin/stdout data transmission
// NO MCP protocol logic, just raw text lines

typedef struct {
    bool line_buffered;
    bool log_to_stderr;
} stdio_simple_config_t;

static int stdio_simple_init(transport_simple_t* transport, void* config) {
    if (!transport) return TRANSPORT_SIMPLE_ERROR;
    
    stdio_simple_config_t* cfg = (stdio_simple_config_t*)config;
    transport->config = config;
    
    // Set line buffering if requested
    if (cfg && cfg->line_buffered) {
        setlinebuf(stdout);
    }
    
    transport->initialized = true;
    transport->connected = true; // STDIO is always "connected"
    
    if (cfg && cfg->log_to_stderr) {
        fprintf(stderr, "[STDIO-SIMPLE] Transport initialized\n");
        fflush(stderr);
    }
    
    return TRANSPORT_SIMPLE_OK;
}

static void stdio_simple_shutdown(transport_simple_t* transport) {
    if (!transport) return;
    
    stdio_simple_config_t* cfg = (stdio_simple_config_t*)transport->config;
    if (cfg && cfg->log_to_stderr) {
        fprintf(stderr, "[STDIO-SIMPLE] Transport shutdown\n");
        fflush(stderr);
    }
    
    transport->initialized = false;
    transport->connected = false;
}

static int stdio_simple_connect(transport_simple_t* transport) {
    if (!transport) return TRANSPORT_SIMPLE_ERROR;
    transport->connected = true; // STDIO is always available
    return TRANSPORT_SIMPLE_OK;
}

static int stdio_simple_disconnect(transport_simple_t* transport) {
    if (!transport) return TRANSPORT_SIMPLE_ERROR;
    transport->connected = false;
    return TRANSPORT_SIMPLE_OK;
}

static int stdio_simple_send_raw(transport_simple_t* transport, const char* data, size_t len) {
    if (!transport || !data || len == 0 || !transport->connected) {
        return TRANSPORT_SIMPLE_ERROR;
    }
    
    // Pure data transmission - write exactly what we're given
    size_t written = fwrite(data, 1, len, stdout);
    fflush(stdout);
    
    return (written == len) ? TRANSPORT_SIMPLE_OK : TRANSPORT_SIMPLE_ERROR;
}

static int stdio_simple_recv_raw(transport_simple_t* transport, char* buffer, size_t buffer_size, int timeout_ms) {
    if (!transport || !buffer || buffer_size == 0 || !transport->connected) {
        return TRANSPORT_SIMPLE_ERROR;
    }
    
    // Use select for timeout
    fd_set readfds;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
    if (result <= 0) {
        return TRANSPORT_SIMPLE_TIMEOUT;
    }
    
    // Read raw data from stdin
    if (!fgets(buffer, buffer_size, stdin)) {
        return TRANSPORT_SIMPLE_DISCONNECTED;
    }
    
    return TRANSPORT_SIMPLE_OK;
}

static bool stdio_simple_is_connected(transport_simple_t* transport) {
    return transport && transport->connected;
}

static void stdio_simple_on_error(transport_simple_t* transport, transport_simple_result_t error, const char* message) {
    stdio_simple_config_t* cfg = (stdio_simple_config_t*)transport->config;
    if (cfg && cfg->log_to_stderr) {
        fprintf(stderr, "[STDIO-SIMPLE] ERROR %d: %s\n", error, message ? message : "Unknown error");
        fflush(stderr);
    }
}

// Factory function for STDIO transport
transport_simple_t* transport_simple_create_stdio(void) {
    transport_simple_t* transport = malloc(sizeof(transport_simple_t));
    if (!transport) return NULL;
    
    memset(transport, 0, sizeof(transport_simple_t));
    
    transport->type = TRANSPORT_SIMPLE_STDIO;
    strncpy(transport->name, "stdio", sizeof(transport->name) - 1);
    
    // Set function pointers
    transport->init = stdio_simple_init;
    transport->shutdown = stdio_simple_shutdown;
    transport->connect = stdio_simple_connect;
    transport->disconnect = stdio_simple_disconnect;
    transport->send_raw = stdio_simple_send_raw;
    transport->recv_raw = stdio_simple_recv_raw;
    transport->is_connected = stdio_simple_is_connected;
    transport->on_error = stdio_simple_on_error;
    
    return transport;
}