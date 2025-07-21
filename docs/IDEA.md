# RKLLM MCP Server - System Architecture

## Overview

This is an MCP (Model Context Protocol) server that wraps the RKLLM library from Rockchip, providing real-time streaming communication across multiple transport channels. The system maps all functions, parameters, constants, variables, and states from the RKLLM C library to the MCP protocol (JSON-RPC 2.0), running multiple transport servers (stdio, UDP, TCP, HTTP, WebSocket) concurrently.

### Key Features

- **Multi-transport support**: All transports can run simultaneously on different ports
- **Unified protocol**: All transports use the same MCP protocol standard
- **User-configurable**: System is fully customizable through auto-generated `settings.json`
- **Universal client support**: Any software that can communicate via stdio/TCP/UDP/HTTP/WS can be a client

### Supported Clients

- Other agents
- Other instances of this server
- Software written in any programming language or environment

## Real-time Streaming Design

### When Streaming is Needed

Streaming is only required when calling the `rkllm_run_async` function in the RKLLM library.

### The Challenge

MCP protocol is based on JSON-RPC 2.0, which doesn't natively support real-time data streaming. The challenge is how to stream token chunks back to the client when `rkllm_run_async` is called.

## Streaming Implementation

### For Real-time Transports (stdio, TCP, UDP, WebSocket)

These protocols support bidirectional real-time communication after connection establishment.

**Flow:**
1. Client sends standard MCP request with `rkllm_run_async` method
2. Server calls corresponding RKLLM function
3. When RKLLM returns tokens, it triggers the pre-registered callback
4. Callback forwards data to MCP layer for JSON packaging and sends to transport

### For HTTP Transport

HTTP requires a polling mechanism due to its request-response nature.

**Flow:**
1. Client sends standard MCP request with `rkllm_run_async` method
2. Server calls corresponding RKLLM function
3. Tokens from RKLLM are buffered in HTTP-specific storage
4. Client polls using the original request ID to retrieve accumulated tokens
5. Buffer is cleared after each poll or after 30-second timeout
6. Polling continues until the final chunk is received

**Key Differences:**
- Real-time transports: Direct streaming, no buffering needed
- HTTP transport: Requires buffering and polling mechanism

## Data Format Specifications

### Standard Request (Method Call)
```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "method": "rkllm_run_async",
  "params": { /* method parameters */ }
}
```

### Polling Request (HTTP only)
```json
{
  "jsonrpc": "2.0", 
  "id": 123,
  "method": "poll",
  "params": {}
}
```

### Standard Response (Non-streaming methods)
```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "result": {
    /* response data */
  }
}
```

### Streaming Chunk (Real-time transports)
```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "result": {
    "chunk": {
      "seq": 0,
      "delta": "Hello",
      "end": false
    }
  }
}
```

### Polling Response (HTTP transport)
```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "result": {
    "chunk": {
      "seq": 0,
      "delta": "Hello friend, how",
      "end": true
    }
  }
}
```

### Chunk Format Rules

- **seq**: Sequential number for ordering chunks (starts from 0)
- **delta**: Content of the chunk (single token for real-time, concatenated tokens for HTTP)
- **end**: Boolean flag indicating the final chunk (only present when `true`)

## End-to-End Data Flow

1. **Connection**: Client connects using any supported transport
2. **Request**: Client sends JSON-RPC request to server transport
3. **Transport Layer**: Server transport forwards message to MCP layer
4. **MCP Processing**: MCP layer parses JSON and forwards to RKLLM proxy
5. **RKLLM Execution**: Proxy calls target function in RKLLM library
6. **Callback**: RKLLM calls registered callback function (buffer manager)
7. **Buffer Management**: 
   - HTTP: Accumulate tokens in buffer until client polls
   - Others: Forward directly to MCP layer
8. **MCP Packaging**: Format data as MCP-compliant JSON
9. **Response**: Transport sends formatted response back to client

## Configuration

The system is configured through `settings.json`, which is auto-generated on server startup. User settings take precedence over default code settings. Default settings should eventually be moved to a header file for better organization.