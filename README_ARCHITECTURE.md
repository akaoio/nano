# MCP Server - Unified Architecture

## Overview

MCP Server is a unified Model Context Protocol server that provides clean separation between transport layer and protocol layer.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        MCP Server                              │
├─────────────────────────────────────────────────────────────────┤
│  main.c                                                         │
│  └── server/server.c (Unified Server)                          │
│      ├── protocol/mcp_adapter.c (MCP Protocol Logic)           │
│      ├── transport/transport_manager.c (Integration Layer)     │
│      ├── operations.c (Business Logic)                         │
│      └── transport/ (Pure Data Transmission)                   │
│          ├── stdio_transport/                                  │
│          ├── tcp_transport/                                    │
│          ├── udp_transport/                                    │
│          ├── http_transport/                                   │
│          └── ws_transport/                                     │
└─────────────────────────────────────────────────────────────────┘
```

## Key Benefits

✅ **Unified Architecture**: Clean server architecture  
✅ **Single Responsibility**: Each layer has one clear purpose  
✅ **Clean Separation**: Transport vs Protocol vs Business Logic  
✅ **Easy to Understand**: Clear "server" concept  
✅ **Maintainable**: Changes in one place only  

## Directory Structure

```
src/
├── main.c                          # Main entry point
├── server/                         # Unified server implementation
│   ├── server.h/c                  # Main server coordination
│   ├── protocol/                   # MCP protocol handling
│   │   └── mcp_adapter.h/c         # Single source of MCP logic
│   ├── transport/                  # Transport layer
│   │   ├── transport_manager.h/c   # Integration layer
│   │   ├── stdio_transport/        # STDIO implementation
│   │   ├── tcp_transport/          # TCP implementation
│   │   ├── udp_transport/          # UDP implementation
│   │   ├── http_transport/         # HTTP implementation
│   │   └── ws_transport/           # WebSocket implementation
│   ├── operations.h/c              # Business logic operations
│   ├── core/                       # Core functionality
│   └── system/                     # System utilities
├── common/                         # Shared utilities
└── libs/                          # External libraries
```

## Data Flow

```
Client Request → Transport Layer → Transport Manager → MCP Adapter → Operations → Response
```

1. **Client** sends request via any transport (STDIO, TCP, UDP, HTTP, WebSocket)
2. **Transport Layer** handles raw data transmission (no protocol awareness)
3. **Transport Manager** coordinates between transport and protocol
4. **MCP Adapter** handles all MCP protocol logic (JSON-RPC, validation, formatting)
5. **Operations** processes the business logic
6. **Response** flows back through the same path

## Transport Layer

Each transport is now **pure data transmission**:

- **STDIO**: Raw stdin/stdout communication
- **TCP**: Raw TCP socket communication  
- **UDP**: Raw UDP socket communication
- **HTTP**: Raw HTTP request/response handling
- **WebSocket**: Raw WebSocket frame handling

**No MCP protocol logic in transports!**

## MCP Protocol Layer

All MCP protocol logic is centralized in `mcp_adapter.c`:

- UTF-8 validation
- JSON-RPC parsing/formatting
- Request/response handling
- Stream management
- Error handling
- Message batching

## Usage

```bash
# Start server with all transports
./server

# Start with specific transports
./server --disable-udp --disable-ws

# Custom ports
./server --tcp 9000 --http 9001

# Help
./server --help
```

## Benefits of Unified Architecture

### Before (Confusing)
- Separate IO and protocol directories
- Unclear boundaries between components
- Duplicated MCP logic across transports
- Hard to understand data flow

### After (Clear)
- Single "server" concept everyone understands
- Clear separation: Transport vs Protocol vs Business
- Single source of MCP protocol logic
- Obvious data flow from client to operations

This architecture makes the codebase much more approachable for new developers with a clear unified server design.