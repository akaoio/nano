# RKLLM Unix Domain Socket Server

A high-performance C server providing direct access to Rockchip's RKLLM AI library through a Unix Domain Socket with JSON-RPC 2.0 interface.

## What

This project implements a lightweight, ultra-modular C server that exposes the complete RKLLM API through a single Unix Domain Socket. It enables any programming language (Python, JavaScript, Go, Rust, etc.) to interact with Rockchip's AI models via standard JSON-RPC calls.

**Key Features:**
- 🚀 Direct RKLLM library integration with 1:1 API mapping
- ⚡ Real-time streaming with zero-copy data forwarding
- 🔧 JSON-RPC 2.0 protocol for universal client compatibility
- 🏗️ Ultra-modular architecture (one function per file)
- 📡 High-performance epoll-based non-blocking I/O
- 🎯 Support for text generation, LoRA adapters, and logits extraction

## Why

**Problem**: RKLLM library is C-only, making it difficult to integrate with modern development stacks.

**Solution**: This server acts as a universal bridge, allowing any language to access RKLLM functionality through a standard Unix socket interface while maintaining the full performance and feature set of the native library.

## When

Use this server when you need to:
- Access RKLLM from non-C languages
- Build distributed AI applications
- Implement real-time streaming AI features
- Share a single RKLLM model across multiple client applications
- Prototype quickly with RKLLM without C development overhead

## Where

**Deployment Environment:**
- Rockchip NPU-enabled devices (RK3588, etc.)
- Linux systems with RKLLM library support
- Development and production environments requiring AI inference

**Client Compatibility:**
- Any language with Unix Domain Socket support
- Local applications only (no network transport)
- Single-machine deployment model

## How

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libjson-c-dev

# Or manually ensure you have:
# - cmake >= 3.16
# - json-c library
# - pthread support
# - RKLLM library (included in src/external/rkllm/)
```

### Build

```bash
# Clone and build
git clone <repository-url>
cd nano

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# The executable will be: build/rkllm_uds_server
```

### Run

```bash
# From build directory
LD_LIBRARY_PATH=../src/external/rkllm ./rkllm_uds_server

# Server starts and listens on /tmp/rkllm.sock
# Ready for client connections
```

### Quick Test

```bash
# Run the included Node.js test
npm test

# Or run comprehensive tests
node sandbox/comprehensive_lora_logits_test.js
```

## Usage Examples

### Initialize Model

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "rkllm.init",
  "params": [{
    "model_path": "./models/gemma3/model.rkllm",
    "max_context_len": 512,
    "max_new_tokens": 256,
    "temperature": 0.8,
    "top_p": 0.9
  }]
}
```

### Generate Text (Streaming)

```json
{
  "jsonrpc": "2.0",
  "id": 2,
  "method": "rkllm.run_async",
  "params": [
    null,
    {
      "input_type": 0,
      "prompt_input": "Hello, how are you today?"
    },
    {
      "mode": 0,
      "keep_history": 1
    },
    null
  ]
}
```

### Load LoRA Adapter

```json
{
  "jsonrpc": "2.0",
  "id": 3,
  "method": "rkllm.load_lora",
  "params": [{
    "lora_adapter_path": "./models/lora/lora.rkllm",
    "lora_adapter_name": "custom_adapter",
    "scale": 1.0
  }]
}
```

## Architecture

### Core Design
- **Single Transport**: Unix Domain Socket (`/tmp/rkllm.sock`)
- **One Function Per File**: Ultra-modular C implementation
- **Zero Abstraction**: Direct RKLLM API mapping
- **Event-Driven**: epoll-based non-blocking I/O

### Directory Structure
```
src/
├── main.c              # Server entry point
├── server/             # Socket management functions
├── connection/         # Client connection handling
├── jsonrpc/           # JSON-RPC protocol implementation
├── rkllm/             # RKLLM library integration
├── buffer/            # I/O buffer management
└── utils/             # Utility functions
```

### Data Flow
```
Client App → Unix Socket → JSON-RPC → RKLLM Library
    ↑                                        ↓
    ← Real-time Streaming ← Callbacks ←────────
```

## Configuration

Environment variables:
- `RKLLM_UDS_PATH`: Socket path (default: `/tmp/rkllm.sock`)
- `RKLLM_MAX_CLIENTS`: Max concurrent connections

## Limitations

- **One Model**: Only one RKLLM model can be loaded at a time (NPU constraint)
- **Local Only**: Unix Domain Socket limits to single-machine deployment
- **Linux Only**: Requires Linux environment with RKLLM support

## Development

### Adding New Functions

1. Create directory: `src/category/function_name/`
2. Add files: `function_name.c` and `function_name.h`
3. Follow the one-function-per-file rule
4. CMake auto-discovers new files

### Testing

```bash
# Run all tests
make test

# Individual function tests in tests/ directory
```

## Documentation

- `docs/DESIGN.md` - Complete architecture and API reference
- `docs/INSTRUCTIONS.md` - Detailed build and usage instructions
- `docs/LORA_AND_LOGITS_GUIDE.md` - Advanced features guide

## License

MIT License - See LICENSE file

## Contributing

1. Follow the one-function-per-file architecture rule
2. Maintain 1:1 RKLLM API mapping
3. Add tests for new functions
4. Update documentation accordingly

---

**Status**: Production ready for Rockchip NPU devices with RKLLM support.