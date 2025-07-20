#include "base.h"
#include <string.h>
#include <stdio.h>

transport_error_t transport_base_init(transport_base_t* base, const char* name) {
    if (!base || !name) {
        return TRANSPORT_INVALID_PARAM;
    }
    
    memset(base, 0, sizeof(transport_base_t));
    strncpy(base->name, name, sizeof(base->name) - 1);
    base->name[sizeof(base->name) - 1] = '\0';
    base->state = TRANSPORT_STATE_DISCONNECTED;
    base->initialized = true;
    base->running = false;
    
    return TRANSPORT_OK;
}

void transport_base_shutdown(transport_base_t* base) {
    if (!base || !base->initialized) {
        return;
    }
    
    if (base->disconnect && base->state == TRANSPORT_STATE_CONNECTED) {
        base->disconnect(base);
    }
    
    if (base->shutdown) {
        base->shutdown(base);
    }
    
    base->initialized = false;
    base->running = false;
    base->state = TRANSPORT_STATE_DISCONNECTED;
}

transport_error_t transport_base_send_json_rpc(transport_base_t* base, const char* json_rpc) {
    if (!base || !json_rpc) {
        return TRANSPORT_INVALID_PARAM;
    }
    
    if (!base->initialized) {
        return TRANSPORT_ERROR;
    }
    
    if (base->state != TRANSPORT_STATE_CONNECTED) {
        if (base->connect) {
            int ret = base->connect(base);
            if (ret != 0) {
                return TRANSPORT_NOT_CONNECTED;
            }
        } else {
            return TRANSPORT_NOT_CONNECTED;
        }
    }
    
    if (!base->send) {
        return TRANSPORT_ERROR;
    }
    
    size_t len = strlen(json_rpc);
    int ret = base->send(base, json_rpc, len);
    
    if (ret < 0) {
        if (ret == -2) return TRANSPORT_TIMEOUT;
        if (ret == -3) return TRANSPORT_DISCONNECTED;
        return TRANSPORT_ERROR;
    }
    
    return TRANSPORT_OK;
}

transport_error_t transport_base_recv_json_rpc(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms) {
    if (!base || !buffer || buffer_size == 0) {
        return TRANSPORT_INVALID_PARAM;
    }
    
    if (!base->initialized || base->state != TRANSPORT_STATE_CONNECTED) {
        return TRANSPORT_NOT_CONNECTED;
    }
    
    if (!base->recv) {
        return TRANSPORT_ERROR;
    }
    
    int ret = base->recv(base, buffer, buffer_size, timeout_ms);
    
    if (ret < 0) {
        if (ret == -2) return TRANSPORT_TIMEOUT;
        if (ret == -3) return TRANSPORT_DISCONNECTED;
        return TRANSPORT_ERROR;
    }
    
    return TRANSPORT_OK;
}

void transport_base_set_state(transport_base_t* base, transport_state_t new_state) {
    if (!base) return;
    
    transport_state_t old_state = base->state;
    base->state = new_state;
    
    if (base->on_state_change && old_state != new_state) {
        base->on_state_change(base, old_state, new_state);
    }
}

const char* transport_error_to_string(transport_error_t error) {
    switch (error) {
        case TRANSPORT_OK: return "Success";
        case TRANSPORT_ERROR: return "General error";
        case TRANSPORT_TIMEOUT: return "Operation timed out";
        case TRANSPORT_DISCONNECTED: return "Connection lost";
        case TRANSPORT_BUFFER_FULL: return "Buffer full";
        case TRANSPORT_NOT_CONNECTED: return "Not connected";
        case TRANSPORT_INVALID_PARAM: return "Invalid parameter";
        case TRANSPORT_MEMORY_ERROR: return "Memory allocation failed";
        case TRANSPORT_PROTOCOL_ERROR: return "Protocol error";
        default: return "Unknown error";
    }
}