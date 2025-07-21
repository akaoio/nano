# RKLLM MCP Server

A high-performance Model Context Protocol (MCP) server that bridges Rockchip's RKLLM library with any programming language through multiple transport protocols.

## Overview

The RKLLM MCP Server provides universal access to Rockchip's AI acceleration capabilities by wrapping the native RKLLM C library in a JSON-RPC 2.0 compliant server. It supports simultaneous connections through STDIO, TCP, UDP, HTTP, and WebSocket transports, enabling seamless integration with applications written in any language.

### Key Features

- 🚀 **Multi-Transport Support**: Connect via STDIO, TCP, UDP, HTTP, or WebSocket
- 🔄 **Real-Time Streaming**: Live token streaming for interactive AI experiences
- 🛠️ **Complete RKLLM API**: All 21+ RKLLM functions exposed through MCP
- ⚙️ **Flexible Configuration**: JSON-based settings with CLI overrides
- 🔌 **Language Agnostic**: Works with JavaScript, Python, Go, Rust, or any language
- 📊 **Production Ready**: Process management, logging, and error handling

## Quick Start

### Prerequisites

- Linux system with Rockchip SoC (tested on RK3588)
- RKLLM library installed
- CMake 3.10+
- GCC/Clang compiler
- Node.js (for testing)

### Building

```bash
# Clone the repository
git clone <repository-url>
cd nano

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)
```

### Running the Server

```bash
# Start with default settings (all transports enabled)
./build/mcp_server

# Start with specific transports
./build/mcp_server --disable-stdio --disable-tcp

# Force kill existing instances
./build/mcp_server --force
```

## Development Sandbox

For developers looking to experiment with new features or test integrations, check out the [`sandbox/`](sandbox/) directory! This is your playground for:

- 🧪 Experimental features and prototypes
- 🔬 Integration testing with various clients
- 🛠️ Development tools and utilities
- 💥 Breaking changes testing

See [`sandbox/README.md`](sandbox/README.md) for detailed guidelines and structure.

## Configuration

The server automatically generates a `settings.json` file on first run with all available options:

```json
{
  "server": {
    "name": "MCP-Server",
    "version": "1.0.0",
    "logging_enabled": true,
    "pid_file": "/tmp/mcp_server.pid"
  },
  "transports": {
    "stdio": {
      "enabled": true
    },
    "tcp": {
      "enabled": true,
      "host": "0.0.0.0",
      "port": 8080
    },
    "http": {
      "enabled": true,
      "host": "0.0.0.0",
      "port": 8082
    },
    "websocket": {
      "enabled": true,
      "host": "0.0.0.0",
      "port": 8083
    }
  },
  "rkllm": {
    "model_path": "models/qwen3/model.rkllm",
    "max_context_len": 512,
    "max_new_tokens": 256,
    "temperature": 0.8,
    "top_k": 40,
    "top_p": 0.9
  }
}
```

## Client Examples

### JavaScript (Node.js)

```javascript
const net = require('net');

// Connect via TCP
const client = net.createConnection({ host: '127.0.0.1', port: 8080 });

// Send request
const request = JSON.stringify({
  jsonrpc: "2.0",
  method: "rkllm_get_functions",
  params: {},
  id: 1
});

client.write(request + '\n');

// Handle response
client.on('data', (data) => {
  const response = JSON.parse(data.toString());
  console.log('Available functions:', response.result);
});
```

### HTTP Request

```bash
curl -X POST http://localhost:8082 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "rkllm_createDefaultParam",
    "params": {},
    "id": 1
  }'
```

### WebSocket (JavaScript)

```javascript
const WebSocket = require('ws');
const ws = new WebSocket('ws://localhost:8083');

ws.on('open', () => {
  ws.send(JSON.stringify({
    jsonrpc: "2.0",
    method: "rkllm_get_constants",
    params: {},
    id: 1
  }));
});

ws.on('message', (data) => {
  console.log('Response:', JSON.parse(data));
});
```

## API Reference

### Available Methods

#### Meta Functions
- `rkllm_get_functions` - List all available RKLLM functions
- `rkllm_get_constants` - Get RKLLM constants and enums
- `rkllm_createDefaultParam` - Create default parameters

#### Model Management
- `rkllm_init` - Initialize model (required before inference)
- `rkllm_destroy` - Clean up and free resources
- `rkllm_load_lora` - Load LoRA adapter
- `rkllm_set_chat_template` - Configure chat template
- `rkllm_set_function_tools` - Set function calling tools

#### Inference
- `rkllm_run` - Synchronous inference
- `rkllm_run_async` - Asynchronous streaming inference
- `rkllm_abort` - Cancel ongoing inference

#### Cache Management
- `rkllm_clear_kv_cache` - Clear key-value cache
- `rkllm_get_kv_cache_size` - Get cache size
- `rkllm_load_prompt_cache` - Load cached prompts
- `rkllm_release_prompt_cache` - Release prompt cache

#### Monitoring
- `rkllm_is_running` - Check inference status
- `rkllm_get_timings` - Get performance metrics
- `rkllm_print_timings` - Print timing information
- `rkllm_print_memorys` - Print memory usage

### Request Format

```json
{
  "jsonrpc": "2.0",
  "method": "rkllm_init",
  "params": {
    "model_path": "models/qwen3/model.rkllm",
    "temperature": 0.7,
    "max_context_len": 512
  },
  "id": 123
}
```

### Response Format

```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "result": {
    "status": "success",
    "handle": "0x7f8b2c001000"
  }
}
```

## Streaming (Work in Progress)

The server supports real-time token streaming for `rkllm_run_async`:

### Real-Time Transports (STDIO, TCP, UDP, WebSocket)
Tokens are sent immediately as they are generated:

```json
{
  "jsonrpc": "2.0",
  "method": "rkllm_run_async",
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

### HTTP Transport (Polling)
Due to HTTP's stateless nature, clients must poll for chunks:

```json
// Initial request
{
  "jsonrpc": "2.0",
  "method": "rkllm_run_async",
  "params": { ... },
  "id": 123
}

// Poll for chunks
{
  "jsonrpc": "2.0",
  "method": "poll",
  "params": {},
  "id": 123
}
```

## Development Status

Current implementation: **~30% complete**

### ✅ Working
- Multi-transport architecture
- JSON-RPC 2.0 protocol handling
- Settings system with hot reload
- Meta functions (get_functions, get_constants, createDefaultParam)
- Basic HTTP and WebSocket transports
- Process management and port conflict detection

### 🚧 In Progress
- Asynchronous operation handling for long-running tasks
- Real-time streaming implementation
- Complete transport connection management
- Error handling and recovery

### ❌ TODO
- Production logging system
- Performance monitoring
- Security (authentication/authorization)
- Client libraries for popular languages
- Docker containerization
- Comprehensive documentation

## Troubleshooting

### Server won't start
- Check if another instance is running: `ps aux | grep mcp_server`
- Use `--force` flag to kill existing instances
- Verify port availability: `netstat -tulpn | grep 808`

### Connection refused
- Ensure the server is running
- Check firewall settings
- Verify transport is enabled in settings.json

### Model initialization fails
- Verify model file exists and is readable
- Check available memory
- Ensure RKLLM library is properly installed

## Contributing

Contributions are welcome! Please read our contributing guidelines and submit pull requests to the main branch.

## License

[License information to be added]

## Acknowledgments

- Rockchip for the RKLLM library
- Anthropic for the Model Context Protocol specification
- The open-source community for various dependencies

---

For detailed technical documentation, see the [Product Requirements Document](tmp/PRD.md) and [Technical Design](tmp/IDEA.md).