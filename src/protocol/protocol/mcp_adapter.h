#ifndef NANO_MCP_ADAPTER_H
#define NANO_MCP_ADAPTER_H

#include "../transport/transport_base.h"
#include "../transport/streaming/stream_manager.h"
#include "../../io/operations.h"
#include <stdint.h>
#include <stdbool.h>

// MCP Protocol Adapter - Single source of truth for MCP compliance
// All transports use this layer for protocol handling

typedef struct {
    char request_id[32];
    char method[64];
    char params[4096];
    bool is_streaming;
    char* stream_id;
} mcp_request_t;

typedef struct {
    char request_id[32];
    bool is_success;
    char result[4096];
    char error_code[16];
    char error_message[256];
    bool is_streaming_response;
    char* stream_id;
} mcp_response_t;

typedef struct {
    char stream_id[32];
    char method[64];
    uint32_t seq;
    char delta[2048];
    bool end;
    char* error_message;
} mcp_stream_chunk_t;

// MCP Protocol Adapter API
typedef struct {
    bool initialized;
    char protocol_version[16];
    bool utf8_validation_enabled;
    bool message_batching_enabled;
    
    // Statistics
    uint32_t requests_processed;
    uint32_t responses_sent;
    uint32_t stream_chunks_sent;
    uint32_t errors_handled;
} mcp_adapter_t;

// Initialization
int mcp_adapter_init(mcp_adapter_t* adapter);
void mcp_adapter_shutdown(mcp_adapter_t* adapter);

// Request Processing (Transport → MCP → IO)
int mcp_adapter_parse_request(const char* raw_data, mcp_request_t* request);
int mcp_adapter_validate_request(const mcp_request_t* request);
int mcp_adapter_process_request(const mcp_request_t* request, mcp_response_t* response);

// Response Formatting (IO → MCP → Transport)
int mcp_adapter_format_response(const mcp_response_t* response, char* output, size_t output_size);
int mcp_adapter_format_error(const char* request_id, int error_code, const char* message, char* output, size_t output_size);

// Streaming Support
int mcp_adapter_create_stream(const char* method, const char* request_id, char* stream_id_out);
int mcp_adapter_format_stream_chunk(const mcp_stream_chunk_t* chunk, char* output, size_t output_size);
int mcp_adapter_handle_stream_request(const mcp_request_t* request, mcp_response_t* response);

// Message Batching
int mcp_adapter_parse_batch(const char* raw_data, mcp_request_t* requests, size_t* count, size_t max_count);
int mcp_adapter_format_batch_response(const mcp_response_t* responses, size_t count, char* output, size_t output_size);

// Validation
int mcp_adapter_validate_utf8(const char* data);
int mcp_adapter_validate_json_rpc(const char* data);

// Error Codes
#define MCP_ADAPTER_OK 0
#define MCP_ADAPTER_ERROR_INVALID_JSON -1
#define MCP_ADAPTER_ERROR_INVALID_UTF8 -2
#define MCP_ADAPTER_ERROR_MISSING_FIELD -3
#define MCP_ADAPTER_ERROR_INVALID_METHOD -4
#define MCP_ADAPTER_ERROR_STREAM_ERROR -5

// Global adapter instance
extern mcp_adapter_t g_mcp_adapter;

#endif