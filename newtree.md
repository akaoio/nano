nano/
├── src/
│   ├── libs/                    # External libraries
│   │   └── rkllm/
│   │       ├── rkllm.h
│   │       └── librkllmrt.so
│   │
│   ├── io/                      # PURE QUEUE + RKLLM MAPPING
│   │   ├── core/                # Core queue functionality
│   │   │   ├── queue/           # Queue component
│   │   │   │   ├── queue.c      # <100 lines
│   │   │   │   └── queue.h
│   │   │   ├── worker_pool/     # Worker pool component
│   │   │   │   ├── worker_pool.c # <100 lines
│   │   │   │   └── worker_pool.h
│   │   │   ├── io/              # Main IO interface
│   │   │   │   ├── io.c         # <100 lines
│   │   │   │   └── io.h
│   │   │   └── core.h           # Core module interface
│   │   │
│   │   ├── mapping/             # Direct RKLLM mapping
│   │   │   ├── handle_pool/     # Handle pool component
│   │   │   │   ├── handle_pool.c # <100 lines
│   │   │   │   └── handle_pool.h
│   │   │   ├── rkllm_proxy/     # RKLLM proxy component
│   │   │   │   ├── rkllm_proxy.c # <100 lines
│   │   │   │   └── rkllm_proxy.h
│   │   │   └── mapping.h        # Mapping module interface
│   │   │
│   │   └── io_module.h          # IO module interface
│   │
│   ├── nano/                    # BUSINESS LOGIC + TRANSPORT
│   │   ├── core/                # Core nano functionality
│   │   │   ├── nano/            # Main nano component
│   │   │   │   ├── nano.c       # <100 lines
│   │   │   │   └── nano.h
│   │   │   └── core.h           # Core module interface
│   │   │
│   │   ├── validation/          # All validation logic
│   │   │   ├── model_checker/   # Model checking component
│   │   │   │   ├── model_checker.c # <100 lines
│   │   │   │   └── model_checker.h
│   │   │   └── validation.h     # Validation module interface
│   │   │
│   │   ├── system/              # System info and resource management
│   │   │   ├── system_info/     # System information component
│   │   │   │   ├── system_info.c # <100 lines
│   │   │   │   └── system_info.h
│   │   │   ├── resource_mgr/    # Resource manager component
│   │   │   │   ├── resource_mgr.c # <100 lines
│   │   │   │   └── resource_mgr.h
│   │   │   └── system.h         # System module interface
│   │   │
│   │   ├── transport/           # MCP transport layers
│   │   │   ├── mcp_base/        # Base MCP functionality
│   │   │   │   ├── mcp_base.c   # <100 lines
│   │   │   │   └── mcp_base.h
│   │   │   ├── udp_transport/   # UDP transport component
│   │   │   │   ├── udp_transport.c # <100 lines
│   │   │   │   └── udp_transport.h
│   │   │   ├── tcp_transport/   # TCP transport component
│   │   │   │   ├── tcp_transport.c # <100 lines
│   │   │   │   └── tcp_transport.h
│   │   │   ├── http_transport/  # HTTP transport component
│   │   │   │   ├── http_transport.c # <100 lines
│   │   │   │   └── http_transport.h
│   │   │   ├── ws_transport/    # WebSocket transport component
│   │   │   │   ├── ws_transport.c # <100 lines
│   │   │   │   └── ws_transport.h
│   │   │   ├── stdio_transport/ # STDIO transport component
│   │   │   │   ├── stdio_transport.c # <100 lines
│   │   │   │   └── stdio_transport.h
│   │   │   └── transport.h      # Transport module interface
│   │   │
│   │   └── nano_module.h        # Nano module interface
│   │
│   ├── common/                  # Shared utilities
│   │   ├── json_utils/          # JSON utilities component
│   │   │   ├── json_utils.c     # <100 lines
│   │   │   └── json_utils.h
│   │   ├── memory_utils/        # Memory utilities component
│   │   │   ├── memory_utils.c   # <100 lines
│   │   │   └── memory_utils.h
│   │   ├── string_utils/        # String utilities component
│   │   │   ├── string_utils.c   # <100 lines
│   │   │   └── string_utils.h
│   │   └── common.h             # Common utilities interface
│   │
│   └── main.c                   # <100 lines - entry point
│
├── tests/
│   ├── io/                      # Pure IO tests
│   │   ├── test_queue/          # Queue tests
│   │   │   ├── test_queue.c     # <100 lines
│   │   │   └── test_queue.h
│   │   ├── test_worker_pool/    # Worker pool tests
│   │   │   ├── test_worker_pool.c # <100 lines
│   │   │   └── test_worker_pool.h
│   │   ├── test_handle_pool/    # Handle pool tests
│   │   │   ├── test_handle_pool.c # <100 lines
│   │   │   └── test_handle_pool.h
│   │   └── test_io/             # IO integration tests
│   │       ├── test_io.c        # <100 lines
│   │       └── test_io.h
│   │
│   ├── nano/                    # Nano functionality tests
│   │   ├── test_validation/     # Validation tests
│   │   │   ├── test_validation.c # <100 lines
│   │   │   └── test_validation.h
│   │   ├── test_system/         # System tests
│   │   │   ├── test_system.c    # <100 lines
│   │   │   └── test_system.h
│   │   ├── test_transport/      # Transport tests
│   │   │   ├── test_transport.c # <100 lines
│   │   │   └── test_transport.h
│   │   └── test_nano/           # Nano integration tests
│   │       ├── test_nano.c      # <100 lines
│   │       └── test_nano.h
│   │
│   └── integration/             # End-to-end tests
│       ├── test_qwenvl/         # QwenVL integration tests
│       │   ├── test_qwenvl.c    # <100 lines
│       │   └── test_qwenvl.h
│       ├── test_lora/           # LoRA integration tests
│       │   ├── test_lora.c      # <100 lines
│       │   └── test_lora.h
│       └── test_mcp/            # MCP integration tests
│           ├── test_mcp.c       # <100 lines
│           └── test_mcp.h
│
└── Makefile                     # Single root Makefile