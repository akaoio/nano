# RKLLM Unix Domain Socket Server - Complete Design Document

## Executive Summary

The RKLLM Unix Domain Socket Server is a high-performance, ultra-modular C implementation that provides direct access to Rockchip's RKLLM library through a single Unix Domain Socket. The system eliminates transport complexity while maintaining real-time streaming capabilities through a pure JSON-RPC 2.0 interface with 1:1 RKLLM API mapping.

## Development Rules (MANDATORY)

### Rule #1: One Function Per File
**Each function must be in its own dedicated file.**

### Rule #2: No Two Functions in One File  
**Absolutely no exceptions. One file = One function.**

### Rule #3: Directory/File Naming Convention
**Format: `<name>/<name>.<c|h>`**

**Example Structure (C Server Only):**
```
src/
├── server/
│   ├── create_socket/
│   │   ├── create_socket.c
│   │   └── create_socket.h
│   └── accept_connection/
│       ├── accept_connection.c
│       └── accept_connection.h
├── connection/
│   ├── add_connection/
│   │   ├── add_connection.c
│   │   └── add_connection.h
│   └── send_to_connection/
│       ├── send_to_connection.c
│       └── send_to_connection.h
└── rkllm/
    ├── call_rkllm_run_async/
    │   ├── call_rkllm_run_async.c
    │   └── call_rkllm_run_async.h
    └── rkllm_callback/
        ├── rkllm_callback.c
        └── rkllm_callback.h
```

**Note**: This project implements only the C server. External clients (Python, Go, Rust, etc.) connect to this server via Unix Domain Socket.

**These rules are mandatory and non-negotiable. Any deviation will be rejected.**

## System Architecture

### Core Design Principles
1. **Single Transport**: Unix Domain Socket only (/tmp/rkllm.sock)
2. **Direct RKLLM Integration**: No proxy abstraction, direct function calls
3. **Pure JSON-RPC 2.0**: No MCP wrapper, industry standard protocol
4. **Event-Driven**: epoll-based non-blocking I/O for high concurrency
5. **Zero-Copy Streaming**: Direct callback routing from RKLLM to clients
6. **1:1 RKLLM Mapping**: Exact parameter names, types, and structures

### Data Flow Architecture
```
┌─────────────────────────────────────────────────────────────┐
│                        CLIENT LAYER                         │
├─────────────────────────────────────────────────────────────┤
│  Any Unix Domain Socket Client (C, Python, Go, Rust, etc)  │
└─────────────────────────────────────────────────────────────┘
                              │
                    ┌─────────▼─────────┐
                    │ Unix Domain Socket │
                    │   /tmp/rkllm.sock  │
                    └─────────┬─────────┘
                              │
                    ┌─────────▼─────────┐
                    │   JSON-RPC 2.0    │
                    │    (Direct)       │
                    └─────────┬─────────┘
                              │
                    ┌─────────▼─────────┐
                    │   RKLLM Library   │
                    │   (Direct Calls)  │
                    └───────────────────┘
```

## Real-Time Streaming Design

### Streaming Flow
1. Client sends JSON-RPC request with `rkllm.run_async` method
2. Server registers callback context linking request to client
3. Server calls `rkllm_run_async` with unified callback handler
4. **INSTANT FORWARDING**: When RKLLM callback fires, server immediately sends data back to client
5. **1:1 DATA MAPPING**: All callback data (params, values, keys) mapped exactly as received from RKLLM
6. **ZERO PROCESSING**: No buffering, no transformation, no interpretation - pure passthrough
7. Client receives real-time stream with exact RKLLM structure

### Standard Request Format (Exact RKLLM API Mapping)
```json
{
  "jsonrpc": "2.0",
  "id": 123,
  "method": "rkllm.run_async",
  "params": [
    null,  // LLMHandle (managed by server)
    {      // RKLLMInput* rkllm_input
      "role": "user",
      "enable_thinking": false,
      "input_type": 0,  // RKLLM_INPUT_PROMPT
      "prompt_input": "Hello, how are you?"
    },
    {      // RKLLMInferParam* rkllm_infer_params
      "mode": 0,  // RKLLM_INFER_GENERATE
      "lora_params": null,
      "prompt_cache_params": null,
      "keep_history": 1
    },
    null   // void* userdata (managed by server)
  ]
}
```

### Streaming Response Format (Exact RKLLMResult Structure)
**CRITICAL**: Server sends callback data with **ZERO modification** - exact 1:1 mapping from RKLLM callback to JSON-RPC response.

```json
{
  "jsonrpc": "2.0",
  "id": 123,                     // Same ID from original request
  "result": {
    "text": "Hello",                 // const char* text (exactly as from RKLLM)
    "token_id": 1234,            // int32_t token_id (exactly as from RKLLM)
    "last_hidden_layer": {       // RKLLMResultLastHiddenLayer (exactly as from RKLLM)
      "hidden_states": null,     // const float* hidden_states (raw RKLLM data)
      "embd_size": 768,          // int embd_size (raw RKLLM value)
      "num_tokens": 1            // int num_tokens (raw RKLLM value)
    },
    "logits": {                  // RKLLMResultLogits (exactly as from RKLLM)
      "logits": null,            // const float* logits (raw RKLLM data)
      "vocab_size": 32000,       // int vocab_size (raw RKLLM value)
      "num_tokens": 1            // int num_tokens (raw RKLLM value)
    },
    "perf": {                    // RKLLMPerfStat (exactly as from RKLLM)
      "prefill_time_ms": 10.5,   // float prefill_time_ms (raw RKLLM value)
      "prefill_tokens": 10,      // int prefill_tokens (raw RKLLM value)
      "generate_time_ms": 5.2,   // float generate_time_ms (raw RKLLM value)
      "generate_tokens": 1,      // int generate_tokens (raw RKLLM value)
      "memory_usage_mb": 1024.5  // float memory_usage_mb (raw RKLLM value)
    },
    "_callback_state": 1         // LLMCallState state (raw RKLLM callback parameter)
  }
}
```

**Developer Guarantee**: Every field, every value, every structure mirrors exactly what RKLLM provides - no server interpretation or modification.

## Complete API Reference (1:1 RKLLM Mapping)

### Core Functions
- `rkllm.createDefaultParam` → `rkllm_createDefaultParam()`
- `rkllm.init` → `rkllm_init(LLMHandle* handle, RKLLMParam* param, LLMResultCallback callback)`
- `rkllm.run` → `rkllm_run(LLMHandle handle, RKLLMInput* rkllm_input, RKLLMInferParam* rkllm_infer_params, void* userdata)`
- `rkllm.run_async` → `rkllm_run_async(LLMHandle handle, RKLLMInput* rkllm_input, RKLLMInferParam* rkllm_infer_params, void* userdata)`
- `rkllm.abort` → `rkllm_abort(LLMHandle handle)`
- `rkllm.is_running` → `rkllm_is_running(LLMHandle handle)`
- `rkllm.clear_kv_cache` → `rkllm_clear_kv_cache(LLMHandle handle, int keep_system_prompt, int* start_pos, int* end_pos)`
- `rkllm.get_kv_cache_size` → `rkllm_get_kv_cache_size(LLMHandle handle, int* cache_sizes)`
- `rkllm.destroy` → `rkllm_destroy(LLMHandle handle)`
- `rkllm.load_lora` → `rkllm_load_lora(LLMHandle handle, RKLLMLoraAdapter* lora_adapter)`
- `rkllm.load_prompt_cache` → `rkllm_load_prompt_cache(LLMHandle handle, const char* prompt_cache_path)`
- `rkllm.release_prompt_cache` → `rkllm_release_prompt_cache(LLMHandle handle)`
- `rkllm.set_chat_template` → `rkllm_set_chat_template(LLMHandle handle, const char* system_prompt, const char* prompt_prefix, const char* prompt_postfix)`
- `rkllm.set_function_tools` → `rkllm_set_function_tools(LLMHandle handle, const char* system_prompt, const char* tools, const char* tool_response_str)`
- `rkllm.set_cross_attn_params` → `rkllm_set_cross_attn_params(LLMHandle handle, RKLLMCrossAttnParam* cross_attn_params)`

### Complete Structure Mappings

#### RKLLMParam (Exact rkllm.h mapping)
```json
{
  "model_path": "/path/to/model.rkllm",    // const char*
  "max_context_len": 512,                  // int32_t
  "max_new_tokens": 256,                   // int32_t
  "top_k": 40,                            // int32_t
  "n_keep": 0,                            // int32_t
  "top_p": 0.9,                           // float
  "temperature": 0.8,                      // float
  "repeat_penalty": 1.1,                   // float
  "frequency_penalty": 0.0,                // float
  "presence_penalty": 0.0,                 // float
  "mirostat": 0,                          // int32_t
  "mirostat_tau": 5.0,                    // float
  "mirostat_eta": 0.1,                    // float
  "skip_special_token": false,             // bool
  "is_async": false,                       // bool
  "img_start": null,                       // const char*
  "img_end": null,                         // const char*
  "img_content": null,                     // const char*
  "extend_param": {                        // RKLLMExtendParam
    "base_domain_id": 0,                   // int32_t
    "embed_flash": 0,                      // int8_t
    "enabled_cpus_num": 4,                 // int8_t
    "enabled_cpus_mask": 15,               // uint32_t
    "n_batch": 1,                          // uint8_t
    "use_cross_attn": 0,                   // int8_t
    "reserved": null                       // uint8_t[104] - reserved array
  }
}
```

#### RKLLMInput (Union Structure)
```json
{
  "role": "user",                          // const char*
  "enable_thinking": false,                // bool
  "input_type": 0,                         // RKLLMInputType enum
  // Union - only one of these based on input_type:
  "prompt_input": "Hello, how are you?",   // const char* (type 0)
  // OR
  "embed_input": {                         // RKLLMEmbedInput (type 2)
    "embed": [0.1, 0.2, 0.3],             // float* array
    "n_tokens": 3                          // size_t
  },
  // OR
  "token_input": {                         // RKLLMTokenInput (type 1)
    "input_ids": [1, 2, 3, 4],            // int32_t* array
    "n_tokens": 4                          // size_t
  },
  // OR
  "multimodal_input": {                    // RKLLMMultiModelInput (type 3)
    "prompt": "Describe this image",       // char*
    "image_embed": [/* float array */],    // float*
    "n_image_tokens": 196,                 // size_t
    "n_image": 1,                          // size_t
    "image_width": 224,                    // size_t
    "image_height": 224                    // size_t
  }
}
```

#### RKLLMInferParam
```json
{
  "mode": 0,                               // RKLLMInferMode enum
  "lora_params": {                         // RKLLMLoraParam* or null
    "lora_adapter_name": "adapter1"        // const char*
  },
  "prompt_cache_params": {                 // RKLLMPromptCacheParam* or null
    "save_prompt_cache": 1,                // int
    "prompt_cache_path": "/path/cache"     // const char*
  },
  "keep_history": 1                        // int
}
```

#### RKLLMLoraAdapter
```json
{
  "lora_adapter_path": "/path/to/adapter", // const char*
  "lora_adapter_name": "my_adapter",       // const char*
  "scale": 1.0                             // float
}
```

#### RKLLMCrossAttnParam
```json
{
  "encoder_k_cache": null,                 // float* (array not shown)
  "encoder_v_cache": null,                 // float* (array not shown)
  "encoder_mask": null,                    // float* (array not shown)
  "encoder_pos": null,                     // int32_t* (array not shown)
  "num_tokens": 128                        // int
}
```

#### Enum Values (Exact rkllm.h mapping)
```json
// LLMCallState
{
  "RKLLM_RUN_NORMAL": 0,
  "RKLLM_RUN_WAITING": 1,
  "RKLLM_RUN_FINISH": 2,
  "RKLLM_RUN_ERROR": 3
}

// RKLLMInputType  
{
  "RKLLM_INPUT_PROMPT": 0,
  "RKLLM_INPUT_TOKEN": 1,
  "RKLLM_INPUT_EMBED": 2,
  "RKLLM_INPUT_MULTIMODAL": 3
}

// RKLLMInferMode
{
  "RKLLM_INFER_GENERATE": 0,
  "RKLLM_INFER_GET_LAST_HIDDEN_LAYER": 1,
  "RKLLM_INFER_GET_LOGITS": 2
}
```

## Ultra-Modular Implementation Architecture

Following RULES.md, the server implements strict one-function-per-file architecture:

### Core Server Functions
```
src/server/
├── create_socket/create_socket.c       - Creates Unix domain socket
├── bind_socket/bind_socket.c           - Binds socket to path
├── listen_socket/listen_socket.c       - Sets socket to listen mode
├── accept_connection/accept_connection.c - Accepts new client connections
├── setup_epoll/setup_epoll.c           - Initializes epoll for non-blocking I/O
└── cleanup_socket/cleanup_socket.c     - Cleans up socket resources
```

### Connection Management Functions
```
src/connection/
├── create_connection/create_connection.c - Creates new connection structure
├── add_connection/add_connection.c     - Adds connection to manager
├── remove_connection/remove_connection.c - Removes connection from manager
├── find_connection/find_connection.c   - Finds connection by file descriptor
├── cleanup_connection/cleanup_connection.c - Cleans up connection resources
└── send_to_connection/send_to_connection.c - Sends data to specific connection
```

### JSON-RPC Processing Functions
```
src/jsonrpc/
├── parse_request/parse_request.c       - Parses JSON-RPC request
├── validate_request/validate_request.c - Validates request format
├── format_response/format_response.c   - Formats JSON-RPC response
├── format_error/format_error.c         - Formats JSON-RPC error
├── extract_method/extract_method.c     - Extracts method name
├── extract_params/extract_params.c     - Extracts parameters
└── extract_id/extract_id.c             - Extracts request ID
```

### RKLLM Integration Functions
```
src/rkllm/
├── call_rkllm_createDefaultParam/call_rkllm_createDefaultParam.c - Calls rkllm_createDefaultParam
├── call_rkllm_init/call_rkllm_init.c - Calls rkllm_init  
├── call_rkllm_run/call_rkllm_run.c - Calls rkllm_run
├── call_rkllm_run_async/call_rkllm_run_async.c - Calls rkllm_run_async
├── call_rkllm_abort/call_rkllm_abort.c - Calls rkllm_abort
├── call_rkllm_is_running/call_rkllm_is_running.c - Calls rkllm_is_running
├── call_rkllm_clear_kv_cache/call_rkllm_clear_kv_cache.c - Calls rkllm_clear_kv_cache
├── call_rkllm_get_kv_cache_size/call_rkllm_get_kv_cache_size.c - Calls rkllm_get_kv_cache_size
├── call_rkllm_destroy/call_rkllm_destroy.c - Calls rkllm_destroy
├── call_rkllm_load_lora/call_rkllm_load_lora.c - Calls rkllm_load_lora
├── call_rkllm_load_prompt_cache/call_rkllm_load_prompt_cache.c - Calls rkllm_load_prompt_cache
├── call_rkllm_release_prompt_cache/call_rkllm_release_prompt_cache.c - Calls rkllm_release_prompt_cache
├── call_rkllm_set_chat_template/call_rkllm_set_chat_template.c - Calls rkllm_set_chat_template
├── call_rkllm_set_function_tools/call_rkllm_set_function_tools.c - Calls rkllm_set_function_tools
├── call_rkllm_set_cross_attn_params/call_rkllm_set_cross_attn_params.c - Calls rkllm_set_cross_attn_params
├── rkllm_callback/rkllm_callback.c - Handles RKLLM callbacks
├── convert_json_to_rkllm_param/convert_json_to_rkllm_param.c - Converts JSON to RKLLMParam
├── convert_json_to_rkllm_input/convert_json_to_rkllm_input.c - Converts JSON to RKLLMInput
├── convert_json_to_rkllm_infer_param/convert_json_to_rkllm_infer_param.c - Converts JSON to RKLLMInferParam
├── convert_rkllm_result_to_json/convert_rkllm_result_to_json.c - Converts RKLLMResult to JSON
└── create_callback_context/create_callback_context.c - Creates callback context
```

### Buffer Management Functions
```
src/buffer/
├── create_buffer/create_buffer.c       - Creates I/O buffer
├── resize_buffer/resize_buffer.c       - Resizes buffer if needed
├── append_buffer/append_buffer.c       - Appends data to buffer
├── consume_buffer/consume_buffer.c     - Consumes data from buffer
└── destroy_buffer/destroy_buffer.c     - Destroys buffer
```

### Utility Functions
```
src/utils/
├── log_message/log_message.c           - Logs a message
├── get_timestamp/get_timestamp.c       - Gets current timestamp
├── parse_frame/parse_frame.c           - Parses length-prefixed frame
└── create_frame/create_frame.c         - Creates length-prefixed frame
```

## Benefits of Ultra-Modular Architecture

1. **Perfect Isolation**: Each function completely independent
2. **Individual Testing**: Test every function in isolation
3. **Zero Conflicts**: No merge conflicts between functions
4. **Parallel Development**: Multiple developers work simultaneously
5. **Clear Dependencies**: Function relationships explicit in includes
6. **Easy Debugging**: Issues localized to single function files

## Build System

### CMake Configuration
The CMakeLists.txt automatically discovers all source files following the rule:

```cmake
# Auto-discover all source files following the rule: <name>/<name>.c
file(GLOB_RECURSE SERVER_SOURCES 
    "src/*/*.c"
    "src/main.c"  # Exception: main.c can be at root of src/
)
add_executable(rkllm_uds_server ${SERVER_SOURCES})
```

### Dependencies
- **cmake** >= 3.16
- **json-c** (libjson-c-dev)
- **pthread** (usually included)
- **RKLLM library** (auto-downloaded)

### Quick Build
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libjson-c-dev

# Build
mkdir build && cd build
cmake ..
make

# Run
LD_LIBRARY_PATH=../src/external/rkllm ./rkllm_uds_server
```

## Configuration

The system uses minimal configuration via environment variables:
- `RKLLM_UDS_PATH`: Socket path (default: `/tmp/rkllm.sock`)
- `RKLLM_MAX_CLIENTS`: Maximum concurrent clients

**Hardware Limitations**: 
- Only **ONE model** can be loaded at a time due to NPU memory constraints
- Only **ONE instance** of a model can run at a time
- All clients share the same loaded model instance
- Model switching requires destroying the current model and loading a new one

**Server Philosophy**: 
- The server is a **dumb worker** that only responds to client JSON-RPC requests
- No automatic model loading or pre-configuration
- No default behaviors - clients must explicitly request all operations
- Server waits passively for client commands via `rkllm.init`, `rkllm.run_async`, etc.

## Performance Targets

- **Connection Latency**: <1ms
- **Streaming Latency**: <10ms per token
- **Throughput**: 10,000+ requests/second
- **Concurrent Connections**: 100+ simultaneous clients
- **Memory Efficiency**: 50% less usage than multi-transport version

## Security Features

- Unix socket permissions (0600)
- Path validation for all file operations
- Resource limits per client connection
- Automatic cleanup on client disconnect

## Development Guidelines

### Header File Template
```c
#ifndef FUNCTION_NAME_H
#define FUNCTION_NAME_H

/**
 * Brief description of what the function does
 * @param param1 Description of parameter
 * @return Description of return value
 */
return_type function_name(param_type param1);

#endif
```

### Implementation File Template
```c
#include "function_name.h"
#include <necessary/headers.h>

return_type function_name(param_type param1) {
    // Single function implementation
    // No helper functions allowed
    return result;
}
```

## Testing Strategy

Each function gets individual unit tests following the same structure:
```
tests/
├── server/
│   └── create_socket/
│       └── test_create_socket.c
├── connection/
│   └── add_connection/
│       └── test_add_connection.c
└── rkllm/
    └── rkllm_callback/
        └── test_rkllm_callback.c
```

## Documentation Standards

All documentation must reflect the ultra-modular architecture and one-function-per-file design. No exceptions are allowed to the development rules outlined in this document.

## Conclusion

This design document defines a complete, ultra-modular Unix Domain Socket server for RKLLM integration. The strict architectural rules ensure maximum maintainability, testability, and development efficiency while providing high-performance direct access to the RKLLM library through a standard JSON-RPC 2.0 interface.
