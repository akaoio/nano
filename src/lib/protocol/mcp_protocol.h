#ifndef MCP_PROTOCOL_H
#define MCP_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MCP_PROTOCOL_VERSION "2025-01-07"
#define MCP_JSONRPC_VERSION "2.0"

typedef enum {
    MCP_STATE_DISCONNECTED = 0,
    MCP_STATE_CONNECTED,
    MCP_STATE_INITIALIZING,
    MCP_STATE_INITIALIZED,
    MCP_STATE_ERROR
} mcp_state_t;

typedef enum {
    MCP_ERROR_PARSE = -32700,
    MCP_ERROR_INVALID_REQUEST = -32600,
    MCP_ERROR_METHOD_NOT_FOUND = -32601,
    MCP_ERROR_INVALID_PARAMS = -32602,
    MCP_ERROR_INTERNAL = -32603,
    MCP_ERROR_NOT_INITIALIZED = -32002,
    MCP_ERROR_ALREADY_INITIALIZED = -32003,
    MCP_ERROR_INVALID_VERSION = -32004,
    // Streaming-specific errors
    MCP_ERROR_STREAM_NOT_FOUND = -32001,
    MCP_ERROR_STREAM_EXPIRED = -32005,
    MCP_ERROR_STREAM_INVALID_STATE = -32006
} mcp_error_code_t;

typedef struct {
    char* protocol_version;
    struct {
        bool tools;
        bool resources;
        bool prompts;
        bool sampling;
        bool roots;
        bool progress;
        bool cancellation;
        bool logging;
        bool streaming;
    } capabilities;
    struct {
        char* name;
        char* version;
    } client_info;
    struct {
        char* name;
        char* version;
    } server_info;
    char* instructions;
} mcp_init_data_t;

typedef struct {
    mcp_state_t state;
    uint32_t next_request_id;
    mcp_init_data_t init_data;
    void* user_data;
    
    int (*on_request)(const char* method, const char* params, char* response, size_t response_size);
    int (*on_notification)(const char* method, const char* params);
    void (*on_error)(mcp_error_code_t code, const char* message);
} mcp_context_t;

int mcp_protocol_init(mcp_context_t* ctx);
void mcp_protocol_shutdown(mcp_context_t* ctx);

int mcp_send_initialize(mcp_context_t* ctx, char* buffer, size_t buffer_size);
int mcp_send_initialized(mcp_context_t* ctx, char* buffer, size_t buffer_size);
int mcp_send_ping(mcp_context_t* ctx, char* buffer, size_t buffer_size);

int mcp_handle_message(mcp_context_t* ctx, const char* message, char* response, size_t response_size);

int mcp_format_request(uint32_t id, const char* method, const char* params, char* buffer, size_t buffer_size);
int mcp_format_response(uint32_t id, const char* result, char* buffer, size_t buffer_size);
int mcp_format_error(uint32_t id, mcp_error_code_t code, const char* message, const char* data, char* buffer, size_t buffer_size);
int mcp_format_notification(const char* method, const char* params, char* buffer, size_t buffer_size);

// Streaming support
int mcp_format_stream_chunk(const char* method, const char* stream_id, uint32_t seq, const char* delta, bool end, const char* error_msg, char* buffer, size_t buffer_size);
int mcp_parse_stream_request(const char* params, bool* is_stream, char* other_params, size_t params_size);
int mcp_handle_stream_poll_request(const char* stream_id, uint32_t from_seq, char* response, size_t response_size);

const char* mcp_error_message(mcp_error_code_t code);

#endif