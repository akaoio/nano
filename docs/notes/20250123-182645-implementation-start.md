# Implementation Notes - 2025-01-23 18:26:45

## Current Status
- Read PROMPT.md and DESIGN.md thoroughly
- Started implementing main.c (partially done)
- Need to implement complete ultra-modular architecture

## Key Understanding from DESIGN.md

### Critical Rules (MANDATORY)
1. **One Function Per File** - Each function in dedicated file
2. **No Two Functions in One File** - Absolutely no exceptions
3. **Naming Convention**: `<name>/<name>.<c|h>`

### Architecture Overview
- **Single Transport**: Unix Domain Socket only (/tmp/rkllm.sock)
- **Direct RKLLM Integration**: No proxy, direct function calls
- **Pure JSON-RPC 2.0**: No MCP wrapper
- **Event-Driven**: epoll-based non-blocking I/O
- **Zero-Copy Streaming**: Direct callback routing
- **1:1 RKLLM Mapping**: Exact parameter/structure mapping

### Hardware Constraints
- Only ONE model loaded at a time (NPU memory limitation)
- Only ONE instance can run at a time
- All clients share same model instance
- Model switching requires destroy → load new

### Server Philosophy
- **Dumb worker** - only responds to client requests
- No automatic loading or pre-configuration  
- No default behaviors
- Waits passively for client JSON-RPC commands

### Streaming Design
- **INSTANT FORWARDING**: Callback fires → immediate JSON-RPC response
- **1:1 DATA MAPPING**: Zero transformation, exact RKLLM structure
- **ZERO PROCESSING**: Pure passthrough, no server intelligence

## Implementation Plan
1. ✅ Create main.c (started)
2. ⏳ Implement core server functions (create_socket, bind_socket, etc.)
3. ⏳ Implement connection management 
4. ⏳ Implement JSON-RPC processing
5. ⏳ Implement RKLLM integration
6. ⏳ Implement buffer management
7. ⏳ Implement utility functions
8. ⏳ Test with simple clients
9. ⏳ Validate 1:1 RKLLM mapping

## Next Steps
- Continue implementing server functions following ultra-modular rules
- Test each function individually
- Maintain strict one-function-per-file discipline