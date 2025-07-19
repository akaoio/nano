#pragma once

#include "../../transport/mcp_base/mcp_base.h"
#include "../../../io/operations.h"
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

// C23 compatibility
#if __STDC_VERSION__ >= 202311L
#define NODISCARD [[nodiscard]]
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define NODISCARD __attribute__((warn_unused_result))
#define MAYBE_UNUSED __attribute__((unused))
#endif

// Nano core - Main application logic
// Connects MCP transports to IO subsystem

// Forward declaration for response callback
typedef void (*nano_response_callback_t)(const mcp_message_t* response, void* userdata);

typedef struct {
    bool initialized;
    bool running;
    mcp_transport_t* transports[8];
    int transport_count;
    nano_response_callback_t response_callback;  // Direct callback for responses
    void* response_userdata;                     // Userdata for response callback
} nano_core_t;

// Nano core functions
NODISCARD int nano_init(void);
NODISCARD int nano_add_transport(mcp_transport_t* transport, void* config);
NODISCARD int nano_run(void);
void nano_stop(void);
void nano_shutdown(void);

// Message processing
NODISCARD int nano_process_message(const mcp_message_t* request, mcp_message_t* response);

// Async message processing with callback
NODISCARD int nano_process_message_async(const mcp_message_t* request);

// Set response callback for async processing
void nano_set_response_callback(nano_response_callback_t callback, void* userdata);

// Global nano instance
extern nano_core_t g_nano;
