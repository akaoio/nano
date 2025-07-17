#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// MCP (Model Context Protocol) Base Transport
// Provides common functionality for all MCP transport implementations

// MCP message types
typedef enum {
    MCP_REQUEST,
    MCP_RESPONSE,
    MCP_NOTIFICATION
} mcp_message_type_t;

// MCP message structure
typedef struct {
    mcp_message_type_t type;
    uint32_t id;
    char* method;
    char* params;
    size_t params_len;
} mcp_message_t;

// MCP transport interface
typedef struct {
    int (*init)(void* config);
    int (*send)(const mcp_message_t* message);
    int (*recv)(mcp_message_t* message, int timeout_ms);
    void (*shutdown)(void);
} mcp_transport_t;

// MCP base functions
int mcp_base_init(void);
void mcp_base_shutdown(void);

// Message handling
int mcp_message_create(mcp_message_t* msg, mcp_message_type_t type, uint32_t id, const char* method, const char* params);
void mcp_message_destroy(mcp_message_t* msg);

// JSON RPC helpers
int mcp_parse_json_rpc(const char* json, mcp_message_t* msg);
int mcp_format_json_rpc(const mcp_message_t* msg, char* buffer, size_t buffer_size);
