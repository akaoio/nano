#pragma once

#include "../../transport/mcp_base/mcp_base.h"
#include "../../../io/mapping/rkllm_proxy/rkllm_proxy.h"
#include <stdbool.h>

// Nano core - Main application logic
// Connects MCP transports to IO subsystem

typedef struct {
    bool initialized;
    bool running;
    mcp_transport_t* transports[8];
    int transport_count;
} nano_core_t;

// Nano core functions
int nano_init(void);
int nano_add_transport(mcp_transport_t* transport, void* config);
int nano_run(void);
void nano_stop(void);
void nano_shutdown(void);

// Message processing
int nano_process_message(const mcp_message_t* request, mcp_message_t* response);

// Global nano instance
extern nano_core_t g_nano;
