# RKLLM MCP Server - Real-Time Streaming Tests

This directory contains test clients to verify the real-time streaming functionality across all transport layers.

## 🚀 Quick Start

### 1. Start the RKLLM MCP Server

```bash
# From project root
npm start
# or
./build/mcp_server
```

### 2. Run the Node.js Streaming Test

```bash
# From project root
cd test_client
node nodejs_streaming_client.js
```

## 🌊 What the Test Does

The Node.js client tests real-time streaming across all available transports:

### ✅ **TCP Streaming (Port 8081)**
- Establishes persistent TCP connection
- Sends `rkllm_run_async` request
- Receives real-time streaming chunks as JSON-RPC responses
- Each chunk contains: `seq` (sequence), `delta` (content), `end` (completion flag)

### ✅ **UDP Streaming (Port 8082)**  
- Sends UDP requests
- Receives streaming chunks via UDP packets
- Tests connectionless real-time streaming

### ✅ **HTTP Polling (Port 8080)**
- Sends HTTP POST with `rkllm_run_async`
- Polls server with `poll` method to retrieve buffered chunks
- Implements polling-based streaming for stateless HTTP

### ⚠️ **WebSocket (Disabled)**
- Temporarily disabled due to external ws library dependency
- Can be enabled by installing ws library and rebuilding

## 📊 Expected Output

```
🚀 RKLLM MCP Server - Node.js Streaming Client Test
============================================================

🔌 Testing Basic Connectivity...
✅ HTTP (port 8080): Connected
✅ TCP (port 8081): Connected  
✅ UDP (port 8082): Connected

============================================================
🌊 Starting Streaming Tests...

🚗 Testing TCP Streaming...
✅ Connected to TCP server
📤 Sending request: {"jsonrpc":"2.0","id":1,"method":"rkllm_run_async",...}
📥 Received: {"jsonrpc":"2.0","id":1,"result":{"chunk":{"seq":0,"delta":"Hello","end":false}}}
📥 Received: {"jsonrpc":"2.0","id":1,"result":{"chunk":{"seq":1,"delta":" there","end":false}}}
...
🏁 Stream completed. Total chunks: 15

🛸 Testing UDP Streaming...
📤 Sending UDP request
📥 Received UDP: {"jsonrpc":"2.0","id":2,"result":{"chunk":{"seq":0,"delta":"Once","end":false}}}
...

🌐 Testing HTTP Polling...
📤 Sending HTTP request
📥 Initial HTTP response: {"jsonrpc":"2.0","id":3,"result":{"status":"started"}}
📥 HTTP Polling chunk: {"seq":0,"delta":"Artificial","end":false}
...

============================================================
📊 STREAMING TEST RESULTS
============================================================
✅ TCP: 15 chunks received
✅ UDP: 12 chunks received  
✅ HTTP: 18 chunks received

✨ Testing completed!
```

## 🔧 Architecture Verified

This test verifies the complete real-time streaming pipeline:

1. **RKLLM Integration**: `rkllm_run_async` triggers streaming callback
2. **Stream Manager**: Routes chunks through transport manager  
3. **Transport Layer**: Each transport implements `send_stream_chunk`
4. **MCP Protocol**: Proper JSON-RPC 2.0 streaming format
5. **Multi-Transport**: All transports work simultaneously

## 🐛 Troubleshooting

### Server Not Starting
```bash
# Check if ports are available
netstat -tulpn | grep -E ':(8080|8081|8082|8083)'

# Kill existing processes
pkill mcp_server
```

### Connection Issues
- Ensure firewall allows local connections on test ports
- Check server logs: `tail -f mcp_server.log`
- Verify RKLLM library is properly installed

### No Streaming Chunks
- Confirm RKLLM model is loaded
- Check stream manager initialization
- Verify transport manager global registration

## 📋 Test Client Features

- **Connectivity Testing**: Verifies all transports are accessible
- **Real-Time Streaming**: Tests actual token streaming
- **HTTP Polling**: Implements proper polling mechanism  
- **Error Handling**: Graceful timeout and error management
- **Detailed Logging**: Shows exact request/response flow

This comprehensive test suite validates that the RKLLM MCP Server can successfully stream AI model responses to Node.js clients (or any JSON-RPC 2.0 compatible client) across multiple transport protocols in real-time.