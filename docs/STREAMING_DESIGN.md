# STREAMING MCP PROTOCOL DESIGN

## Overview

This document describes the real-time streaming implementation for the NANO MCP protocol, enabling efficient token-by-token delivery from RKLLM to clients over HTTP and WebSocket transports.

## Design Principles

1. **Transport Agnostic**: Consistent streaming behavior across HTTP and WebSocket
2. **Client Choice**: Clients control streaming via `stream: true` parameter
3. **Reliable Ordering**: Sequential chunk delivery with sequence numbers
4. **Memory Efficient**: Automatic cleanup and session management
5. **Error Resilient**: Graceful error handling and session expiration

## Architecture Components

### 1. Stream Manager (`stream_manager.h/c`)

Central session management for all streaming operations:

```c
typedef struct stream_session {
    char stream_id[17];           // Unique 16-char a-z0-9 identifier
    char original_method[64];     // Original MCP method name
    uint32_t request_id;          // Client's JSON-RPC request ID
    stream_state_t state;         // INITIALIZING/ACTIVE/FINISHED/ERROR
    
    stream_chunk_t* chunk_head;   // Linked list of chunks
    uint32_t next_seq;            // Next sequence number
    uint32_t last_consumed_seq;   // Last consumed sequence (HTTP only)
    
    time_t created_time;          // Session creation timestamp
    time_t last_access_time;      // Last access timestamp
    size_t total_buffer_size;     // Memory usage tracking
} stream_session_t;
```

**Key Features:**
- Automatic session expiration (5 minutes)
- Memory usage monitoring
- Thread-safe chunk management
- Configurable cleanup policies

### 2. HTTP Streaming (Polling-based)

**Client Flow:**
```
1. Client sends: {"method": "run", "params": {"stream": true, ...}}
2. Server responds: {"result": {"stream_id": "abc123", "status": "streaming_started"}}
3. Client polls: {"method": "run", "params": {"stream_id": "abc123", "from_seq": 0}}
4. Server responds: {"result": {"chunks": [...], "has_more": true}}
5. Repeat step 3-4 until has_more: false
```

**Chunk Format:**
```json
{
  "stream_id": "abc123def456",
  "chunks": [
    {
      "seq": 0,
      "delta": "Hello",
      "end": false
    },
    {
      "seq": 1, 
      "delta": " World",
      "end": false
    },
    {
      "seq": 2,
      "delta": "!",
      "end": true
    }
  ],
  "has_more": false
}
```

**Memory Management:**
- Consumed chunks are freed after delivery
- Sessions auto-expire after 5 minutes
- Memory usage tracking prevents overflow

### 3. WebSocket Streaming (Push-based)

**Client Flow:**
```
1. Client sends: {"method": "run", "params": {"stream": true, ...}}
2. Server responds: {"result": {"stream_id": "abc123", "status": "streaming_started"}}
3. Server pushes chunks immediately as they arrive:
```

**Chunk Format:**
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

**Real-time Delivery:**
- Chunks sent immediately via WebSocket
- No polling required
- Lower latency than HTTP
- Persistent connection management

### 4. MCP Protocol Extensions

**New Capabilities:**
```json
{
  "capabilities": {
    "streaming": true
  }
}
```

**Error Codes:**
- `-32001`: Stream session not found or expired  
- `-32005`: Stream session expired
- `-32006`: Stream in invalid state

**Stream Parameter Parsing:**
- Detects `stream: true` in request params
- Removes stream parameter before processing
- Routes to appropriate streaming handler

### 5. RKLLM Integration

**Streaming Callback:**
```c
int io_streaming_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    // Convert RKLLM output to stream chunks
    // Add to stream session buffer
    // Trigger transport-specific delivery
}
```

**State Mapping:**
- `RKLLM_RUN_NORMAL` → Continue streaming
- `RKLLM_RUN_WAITING` → Wait for UTF-8 completion
- `RKLLM_RUN_FINISH` → Final chunk (end: true)
- `RKLLM_RUN_ERROR` → Error chunk with message

## Implementation Details

### Stream ID Generation
- 16 characters: `[a-z0-9]`
- Cryptographically random
- Collision-resistant for reasonable session counts

### Sequence Numbers
- Start at 0
- Increment for each chunk
- Enable client-side ordering verification
- Support gap detection and recovery

### Session Lifecycle
```
INITIALIZING → ACTIVE → FINISHED/ERROR → CLEANUP
     ↓            ↓           ↓             ↓
   Created    Streaming   Complete     Destroyed
```

### Memory Management
- Chunk buffers automatically freed
- Session cleanup on completion/expiration
- Memory usage monitoring and limits
- Graceful degradation under memory pressure

### Error Handling
- Network errors: Retry with exponential backoff
- Session errors: Clean session termination
- Protocol errors: JSON-RPC error responses
- RKLLM errors: Error chunks in stream

## Transport Comparison

| Feature | HTTP Polling | WebSocket Push |
|---------|-------------|----------------|
| Latency | Higher (polling interval) | Lower (immediate) |
| Complexity | Simple | Moderate |
| Reliability | High (stateless) | Good (connection-based) |
| Bandwidth | Higher (polling overhead) | Lower (efficient frames) |
| Client Implementation | Easier | Slightly more complex |
| Server Resources | Higher (session storage) | Lower (no buffering) |

## Usage Examples

### HTTP Streaming Client
```javascript
// 1. Start streaming
const response = await fetch('/mcp', {
  method: 'POST',
  body: JSON.stringify({
    jsonrpc: '2.0',
    id: 1,
    method: 'run', 
    params: {prompt: 'Hello', stream: true}
  })
});
const {result: {stream_id}} = await response.json();

// 2. Poll for chunks
let seq = 0;
while (true) {
  const pollResponse = await fetch('/mcp', {
    method: 'POST',
    body: JSON.stringify({
      jsonrpc: '2.0',
      id: 2,
      method: 'run',
      params: {stream_id, from_seq: seq}
    })
  });
  
  const {result: {chunks, has_more}} = await pollResponse.json();
  
  for (const chunk of chunks) {
    process.stdout.write(chunk.delta);
    seq = chunk.seq + 1;
    if (chunk.end) break;
  }
  
  if (!has_more) break;
  await sleep(100); // Polling interval
}
```

### WebSocket Streaming Client
```javascript
const ws = new WebSocket('ws://localhost:8080/ws');

// 1. Start streaming
ws.send(JSON.stringify({
  jsonrpc: '2.0',
  id: 1,
  method: 'run',
  params: {prompt: 'Hello', stream: true}
}));

// 2. Handle streaming chunks
ws.onmessage = (event) => {
  const message = JSON.parse(event.data);
  
  if (message.id === 1) {
    // Initial response with stream_id
    console.log('Stream started:', message.result.stream_id);
  } else if (message.method === 'run') {
    // Streaming chunk
    const {params: {seq, delta, end}} = message;
    process.stdout.write(delta);
    if (end) console.log('\\nStream complete');
  }
};
```

## Testing

Comprehensive test suite covers:
- Stream manager functionality
- HTTP polling mechanics
- WebSocket push delivery
- MCP protocol extensions
- Error handling scenarios
- Memory management
- Session lifecycle
- Multi-client scenarios

## Performance Characteristics

**Throughput:**
- HTTP: ~100-500 chunks/second (depending on polling frequency)
- WebSocket: ~1000+ chunks/second (limited by processing speed)

**Memory Usage:**
- ~1KB per active session (base overhead)
- ~100 bytes per buffered chunk (HTTP only)
- Automatic cleanup prevents memory leaks

**Latency:**
- HTTP: 50-200ms (polling interval dependent)
- WebSocket: <10ms (near real-time)

## Security Considerations

1. **Stream ID Privacy**: Random IDs prevent session enumeration
2. **Session Expiration**: Automatic cleanup prevents resource exhaustion
3. **Memory Limits**: Bounded buffer sizes prevent DoS attacks
4. **Authentication**: Standard MCP authentication applies
5. **Rate Limiting**: Per-client stream limits recommended

## Future Enhancements

1. **Compression**: Gzip compression for large chunks
2. **Multiplexing**: Multiple streams per connection
3. **Acknowledgments**: Client ACK for reliable delivery
4. **Prioritization**: Priority-based chunk ordering
5. **Persistence**: Optional stream persistence across reconnections

## Compatibility

- **MCP Version**: 2025-01-07 and later
- **JSON-RPC**: 2.0 compliant
- **Transport**: HTTP/1.1, WebSocket (RFC 6455)
- **Encoding**: UTF-8 text streams
- **Browsers**: Modern browsers with WebSocket support