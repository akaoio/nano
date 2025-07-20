#ifndef NANO_TRANSPORT_SIMPLE_H
#define NANO_TRANSPORT_SIMPLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Simplified Transport Interface - Pure data transmission only
// No MCP protocol logic, just raw data in/out

typedef enum {
    TRANSPORT_SIMPLE_OK = 0,
    TRANSPORT_SIMPLE_ERROR = -1,
    TRANSPORT_SIMPLE_TIMEOUT = -2,
    TRANSPORT_SIMPLE_DISCONNECTED = -3,
    TRANSPORT_SIMPLE_BUFFER_FULL = -4
} transport_simple_result_t;

typedef enum {
    TRANSPORT_SIMPLE_STDIO = 0,
    TRANSPORT_SIMPLE_TCP,
    TRANSPORT_SIMPLE_UDP,
    TRANSPORT_SIMPLE_HTTP,
    TRANSPORT_SIMPLE_WEBSOCKET
} transport_simple_type_t;

// Simple transport interface - only data transmission
typedef struct transport_simple {
    transport_simple_type_t type;
    char name[32];
    bool initialized;
    bool connected;
    void* config;
    void* impl_data;
    
    // Core transport functions - NO MCP LOGIC
    int (*init)(struct transport_simple* transport, void* config);
    void (*shutdown)(struct transport_simple* transport);
    int (*connect)(struct transport_simple* transport);
    int (*disconnect)(struct transport_simple* transport);
    
    // Raw data transmission - NO PROTOCOL AWARENESS
    int (*send_raw)(struct transport_simple* transport, const char* data, size_t len);
    int (*recv_raw)(struct transport_simple* transport, char* buffer, size_t buffer_size, int timeout_ms);
    
    // Transport-specific features (optional)
    int (*send_batch)(struct transport_simple* transport, const char** data_array, size_t* len_array, size_t count);
    bool (*is_connected)(struct transport_simple* transport);
    
    // Error handling
    void (*on_error)(struct transport_simple* transport, transport_simple_result_t error, const char* message);
} transport_simple_t;

// Transport factory functions
transport_simple_t* transport_simple_create_stdio(void);
transport_simple_t* transport_simple_create_tcp(const char* host, int port, bool is_server);
transport_simple_t* transport_simple_create_udp(const char* host, int port);
transport_simple_t* transport_simple_create_http(const char* host, int port, const char* path);
transport_simple_t* transport_simple_create_websocket(const char* host, int port, const char* path);

// Transport lifecycle
int transport_simple_init(transport_simple_t* transport, void* config);
void transport_simple_shutdown(transport_simple_t* transport);
int transport_simple_connect(transport_simple_t* transport);
int transport_simple_disconnect(transport_simple_t* transport);

// Raw data transmission (NO MCP AWARENESS)
int transport_simple_send(transport_simple_t* transport, const char* data, size_t len);
int transport_simple_recv(transport_simple_t* transport, char* buffer, size_t buffer_size, int timeout_ms);

// Utility functions
const char* transport_simple_type_to_string(transport_simple_type_t type);
const char* transport_simple_result_to_string(transport_simple_result_t result);

#endif