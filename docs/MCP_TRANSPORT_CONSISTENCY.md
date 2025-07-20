# MCP Transport Consistency Specification

## Overview

All NANO transports (STDIO, TCP, UDP, HTTP, WebSocket) implement identical streaming behavior and MCP protocol compliance, ensuring clients can seamlessly switch between transports without changing application logic.

## Consistent Streaming API

### 1. Stream Request Format
All transports accept identical streaming requests:
```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "method": "run",
  "params": {
    "prompt": "Hello world",
    "stream": true,
    "max_tokens": 100
  }
}
```

### 2. Stream Response Format
All transports return consistent stream initialization responses:
```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "result": {
    "stream_id": "abc123def456",
    "status": "streaming_started",
    "transport": "stdio|tcp|udp|http|ws"
  }
}
```

### 3. Chunk Format
All transports use identical chunk structure:
```json
{
  "jsonrpc": "2.0",
  "method": "run",
  "params": {
    "stream_id": "abc123def456",
    "seq": 0,
    "delta": "Hello",
    "end": false
  }
}
```

## Transport-Specific Delivery Mechanisms

### STDIO Transport
**Delivery**: Push-based via stdout
**Format**: One JSON message per line
**Ordering**: Sequential delivery guaranteed
```bash
# Stream initialization response
{"jsonrpc":"2.0","id":123,"result":{"stream_id":"abc123","status":"streaming_started"}}

# Stream chunks
{"jsonrpc":"2.0","method":"run","params":{"stream_id":"abc123","seq":0,"delta":"Hello","end":false}}
{"jsonrpc":"2.0","method":"run","params":{"stream_id":"abc123","seq":1,"delta":" World","end":false}}
{"jsonrpc":"2.0","method":"run","params":{"stream_id":"abc123","seq":2,"delta":"!","end":true}}
```

### TCP Transport
**Delivery**: Push-based via TCP socket
**Format**: Newline-delimited JSON
**Ordering**: Sequential delivery guaranteed
**Reliability**: TCP ensures delivery
```bash
# Same JSON format as STDIO, sent over TCP socket
{"jsonrpc":"2.0","id":123,"result":{"stream_id":"abc123","status":"streaming_started"}}\n
{"jsonrpc":"2.0","method":"run","params":{"stream_id":"abc123","seq":0,"delta":"Hello","end":false}}\n
```

### UDP Transport
**Delivery**: Push-based via UDP packets
**Format**: One JSON message per UDP packet
**Ordering**: Sequence numbers for reliability
**Reliability**: Retry mechanism + sequence validation
```bash
# UDP adds internal sequence numbers for reliability
{"jsonrpc":"2.0","id":123,"result":{"stream_id":"abc123"},"_udp_seq":1}
{"jsonrpc":"2.0","method":"run","params":{"stream_id":"abc123","seq":0,"delta":"Hello"},"_udp_seq":2}
```

### HTTP Transport
**Delivery**: Pull-based polling
**Format**: Batch chunks in JSON arrays
**Ordering**: Client polls with from_seq parameter
**Reliability**: Stateless HTTP with session management
```bash
# 1. Client polls for chunks
POST /mcp
{"jsonrpc":"2.0","id":2,"method":"run","params":{"stream_id":"abc123","from_seq":0}}

# 2. Server responds with available chunks
{
  "jsonrpc": "2.0",
  "id": 2,
  "result": {
    "stream_id": "abc123",
    "chunks": [
      {"seq": 0, "delta": "Hello", "end": false},
      {"seq": 1, "delta": " World", "end": false}
    ],
    "has_more": true
  }
}
```

### WebSocket Transport
**Delivery**: Push-based via WebSocket frames
**Format**: One JSON message per frame
**Ordering**: Sequential delivery guaranteed
**Reliability**: WebSocket connection management
```bash
# WebSocket frames contain individual JSON messages
{"jsonrpc":"2.0","id":123,"result":{"stream_id":"abc123","status":"streaming_started"}}
{"jsonrpc":"2.0","method":"run","params":{"stream_id":"abc123","seq":0,"delta":"Hello","end":false}}
```

## MCP Protocol Compliance Matrix

| Feature | STDIO | TCP | UDP | HTTP | WS |
|---------|-------|-----|-----|------|-----|
| UTF-8 Validation | ✅ | ✅ | ✅ | ✅ | ✅ |
| No Embedded Newlines | ✅ | ✅ | N/A* | N/A* | N/A* |
| Newline Delimited | ✅ | ✅ | N/A* | N/A* | N/A* |
| JSON-RPC 2.0 | ✅ | ✅ | ✅ | ✅ | ✅ |
| Message Batching | ✅ | ✅ | ✅ | ✅ | ✅ |
| Stream Support | ✅ | ✅ | ✅ | ✅ | ✅ |
| Error Logging | stderr | N/A | N/A | N/A | N/A |

*UDP/HTTP/WS have their own framing mechanisms

## Streaming Session Management

### Consistent Session Lifecycle
1. **Initialization**: `stream_create_session(method, request_id)`
2. **Active**: `stream_add_chunk(stream_id, delta, end, error)`
3. **Completion**: `stream_destroy_session(stream_id)`
4. **Expiration**: 5-minute timeout across all transports

### Stream State Management
```c
typedef enum {
    STREAM_STATE_INITIALIZING = 0,
    STREAM_STATE_ACTIVE,
    STREAM_STATE_FINISHED, 
    STREAM_STATE_ERROR,
    STREAM_STATE_EXPIRED
} stream_state_t;
```

### Memory Management
- **Automatic cleanup** on completion/expiration
- **Memory usage tracking** per session
- **Configurable limits** across all transports

## Client Implementation Guidelines

### Transport-Agnostic Client Code
```typescript
class MCPStreamingClient {
  async startStream(method: string, params: any): Promise<string> {
    const response = await this.send({
      jsonrpc: "2.0",
      id: this.nextId++,
      method,
      params: { ...params, stream: true }
    });
    
    return response.result.stream_id;
  }
  
  // HTTP polling implementation
  async pollHTTPChunks(streamId: string, fromSeq: number = 0): Promise<Chunk[]> {
    const response = await this.send({
      jsonrpc: "2.0", 
      id: this.nextId++,
      method: "run",
      params: { stream_id: streamId, from_seq: fromSeq }
    });
    
    return response.result.chunks;
  }
  
  // Push-based implementation (STDIO/TCP/UDP/WS)
  onMessage(message: any) {
    if (message.method && message.params?.stream_id) {
      // Handle streaming chunk
      this.handleStreamChunk(message.params);
    }
  }
}
```

### Error Handling Consistency
All transports return consistent error formats:
```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "error": {
    "code": -32001,
    "message": "Stream session not found or expired",
    "data": {
      "stream_id": "abc123",
      "transport": "tcp"
    }
  }
}
```

## Performance Characteristics

| Transport | Latency | Throughput | Reliability | Use Case |
|-----------|---------|------------|-------------|----------|
| STDIO | ~1ms | High | Very High | Local processes |
| TCP | ~5ms | Very High | Very High | Network applications |
| UDP | ~2ms | High | Medium* | Low-latency scenarios |
| HTTP | ~50-200ms | Medium | High | Web applications |
| WebSocket | ~10ms | High | High | Real-time web apps |

*UDP reliability enhanced with retry + sequencing

## Testing Consistency

### Unified Test Suite
```c
// Test streaming across all transports
void test_streaming_consistency() {
    transport_t* transports[] = {
        stdio_transport_get_interface(),
        tcp_transport_get_interface(), 
        udp_transport_get_interface(),
        http_transport_get_interface(),
        ws_transport_get_interface()
    };
    
    for (size_t i = 0; i < 5; i++) {
        test_stream_lifecycle(transports[i]);
        test_chunk_ordering(transports[i]);
        test_error_handling(transports[i]);
        test_mcp_compliance(transports[i]);
    }
}
```

### Validation Matrix
- ✅ **Stream initialization** identical across transports
- ✅ **Chunk format** consistent structure  
- ✅ **Error codes** standardized responses
- ✅ **Session management** unified lifecycle
- ✅ **Memory cleanup** automatic across all
- ✅ **MCP compliance** protocol conformance

## Migration Guide

### Switching Between Transports
Applications can switch transports without code changes:

```c
// Development: STDIO
mcp_client_init("stdio://");

// Testing: TCP
mcp_client_init("tcp://localhost:8080");  

// Production: WebSocket
mcp_client_init("ws://localhost:8080/mcp");

// All streaming behavior remains identical
```

### Configuration Examples
```json
{
  "transport": {
    "type": "stdio",
    "config": {
      "streaming_enabled": true,
      "log_to_stderr": true
    }
  }
}
```

```json
{
  "transport": {
    "type": "tcp", 
    "config": {
      "host": "localhost",
      "port": 8080,
      "streaming_enabled": true,
      "enable_retry": true
    }
  }
}
```

## Summary

The NANO implementation ensures **complete streaming consistency** across all five transport mechanisms while respecting each transport's unique characteristics and MCP protocol requirements. Clients can confidently switch between transports based on deployment requirements without changing application logic.