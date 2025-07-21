#ifndef NANO_TRANSPORT_BASE_H
#define NANO_TRANSPORT_BASE_H

#include "common/types.h"
#include <stdint.h>
#include <stdbool.h>

// Default fallback values (use settings when available)
#define TRANSPORT_FALLBACK_BUFFER_SIZE 8192
#define TRANSPORT_FALLBACK_MAX_RETRIES 3
#define TRANSPORT_FALLBACK_DEFAULT_TIMEOUT_MS 5000

typedef enum {
    TRANSPORT_OK = 0,
    TRANSPORT_ERROR = -1,
    TRANSPORT_TIMEOUT = -2,
    TRANSPORT_DISCONNECTED = -3,
    TRANSPORT_BUFFER_FULL = -4,
    TRANSPORT_NOT_CONNECTED = -5,
    TRANSPORT_INVALID_PARAM = -6,
    TRANSPORT_MEMORY_ERROR = -7,
    TRANSPORT_PROTOCOL_ERROR = -8
} transport_error_t;

typedef enum {
    TRANSPORT_STATE_DISCONNECTED = 0,
    TRANSPORT_STATE_CONNECTING,
    TRANSPORT_STATE_CONNECTED,
    TRANSPORT_STATE_ERROR
} transport_state_t;

typedef struct transport_base {
    char name[32];
    transport_state_t state;
    bool initialized;
    bool running;
    
    int (*init)(struct transport_base* base, void* config);
    void (*shutdown)(struct transport_base* base);
    int (*connect)(struct transport_base* base);
    int (*disconnect)(struct transport_base* base);
    int (*send)(struct transport_base* base, const char* data, size_t len);
    int (*recv)(struct transport_base* base, char* buffer, size_t buffer_size, int timeout_ms);
    int (*is_connected)(struct transport_base* base);
    
    void* impl_data;
    
    void (*on_error)(struct transport_base* base, transport_error_t error, const char* msg);
    void (*on_state_change)(struct transport_base* base, transport_state_t old_state, transport_state_t new_state);
} transport_base_t;

transport_error_t transport_base_init(transport_base_t* base, const char* name);
void transport_base_shutdown(transport_base_t* base);
transport_error_t transport_base_send_json_rpc(transport_base_t* base, const char* json_rpc);
transport_error_t transport_base_recv_json_rpc(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
void transport_base_set_state(transport_base_t* base, transport_state_t new_state);
const char* transport_error_to_string(transport_error_t error);

#endif