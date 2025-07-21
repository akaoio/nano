#include "include/mcp/server.h"
#include "common/core.h"
#include "lib/core/process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static mcp_server_t g_server = {0};
static bool g_running = true;

void signal_handler(int sig) {
    (void)sig;
    if (g_running) {
        g_running = false;
        printf("\nShutting down MCP server...\n");
        mcp_server_stop(&g_server);
    }
}

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help           Show this help message\n");
    printf("  -t, --tcp PORT       Set TCP transport port (default: 8080)\n");
    printf("  -u, --udp PORT       Set UDP transport port (default: 8081)\n");
    printf("  -w, --ws PORT        Set WebSocket transport port (default: 8083)\n");
    printf("  -H, --http PORT      Set HTTP transport port (default: 8082)\n");
    printf("  --disable-stdio      Disable STDIO transport\n");
    printf("  --disable-tcp        Disable TCP transport\n");
    printf("  --disable-udp        Disable UDP transport\n");
    printf("  --disable-http       Disable HTTP transport\n");
    printf("  --disable-ws         Disable WebSocket transport\n");
    printf("  --force              Kill existing processes using our ports\n");
    printf("  --log-file FILE      Log to file instead of stderr\n");
    printf("\nDefault configuration:\n");
    printf("STDIO: enabled\n");
    printf("TCP: port 8080\n");
    printf("UDP: port 8081\n");
    printf("HTTP: port 8082\n");
    printf("WebSocket: port 8083\n");
}

int main(int argc, char* argv[]) {
    // We'll determine output stream later based on config
    printf("🚀 MCP Server - Model Context Protocol Server\n");
    printf("====================================================\n");
    
    // Install signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Default configuration
    mcp_server_config_t config = {
        .enable_stdio = true,
        .enable_tcp = true,
        .enable_udp = true,
        .enable_http = true,
        .enable_websocket = true,
        .server_name = "MCP-Server",
        .tcp_port = 8080,
        .udp_port = 8081,
        .http_port = 8082,
        .ws_port = 8083,
        .http_path = "/",
        .ws_path = "/",
        .enable_streaming = true,
        .enable_logging = true,
        .log_file = NULL
    };
    
    bool force_kill = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tcp") == 0) {
            if (i + 1 < argc) {
                config.tcp_port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--udp") == 0) {
            if (i + 1 < argc) {
                config.udp_port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--ws") == 0) {
            if (i + 1 < argc) {
                config.ws_port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--http") == 0) {
            if (i + 1 < argc) {
                config.http_port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--disable-stdio") == 0) {
            config.enable_stdio = false;
        } else if (strcmp(argv[i], "--disable-tcp") == 0) {
            config.enable_tcp = false;
        } else if (strcmp(argv[i], "--disable-udp") == 0) {
            config.enable_udp = false;
        } else if (strcmp(argv[i], "--disable-http") == 0) {
            config.enable_http = false;
        } else if (strcmp(argv[i], "--disable-ws") == 0) {
            config.enable_websocket = false;
        } else if (strcmp(argv[i], "--log-file") == 0) {
            if (i + 1 < argc) {
                config.log_file = argv[++i];
            }
        } else if (strcmp(argv[i], "--force") == 0) {
            force_kill = true;
        }
    }
    
    // Determine output stream based on STDIO transport setting
    FILE* output_stream = config.enable_stdio ? stderr : stdout;
    
    // Redirect initial messages to stderr if STDIO transport is enabled
    if (config.enable_stdio) {
        fprintf(stderr, "🚀 MCP Server - Model Context Protocol Server\n");
        fprintf(stderr, "====================================================\n");
    }
    
    // Check for existing instances
    fprintf(output_stream, "🔍 Checking for existing server instances...\n");
    process_status_t status = process_manager_check_existing();
    
    if (status.is_running) {
        fprintf(output_stream, "⚠️  Found running instance: %s (PID %d)\n", status.process_name, status.pid);
        
        if (force_kill) {
            fprintf(output_stream, "💀 --force specified, terminating existing instance...\n");
            if (process_manager_kill_process(status.pid, true) != 0) {
                fprintf(stderr, "❌ Failed to kill existing instance\n");
                return 1;
            }
        } else {
            fprintf(stderr, "❌ Server already running (PID %d). Use --force to kill it.\n", status.pid);
            return 1;
        }
    }
    
    // Scan for port conflicts
    process_port_scan_t ports[] = {
        {config.tcp_port, "TCP", config.enable_tcp},
        {config.udp_port, "UDP", config.enable_udp},
        {config.http_port, "HTTP", config.enable_http},
        {config.ws_port, "WebSocket", config.enable_websocket}
    };
    
    process_conflict_t conflicts[20];
    int conflict_count = process_manager_scan_ports(ports, 4, conflicts, 20);
    
    if (conflict_count > 0) {
        fprintf(output_stream, "⚠️  Found %d port conflicts:\n", conflict_count);
        for (int i = 0; i < conflict_count; i++) {
            fprintf(output_stream, "   • Port %d (%s): used by %s (PID %d)\n", 
                   conflicts[i].port, conflicts[i].transport_name, 
                   conflicts[i].process_name, conflicts[i].pid);
        }
        
        if (force_kill) {
            fprintf(output_stream, "💀 --force specified, killing conflicting processes...\n");
            int killed = process_manager_kill_conflicts(conflicts, conflict_count, true);
            fprintf(output_stream, "✅ Killed %d of %d conflicting processes\n", killed, conflict_count);
        } else {
            fprintf(stderr, "❌ Port conflicts detected. Use --force to kill conflicting processes.\n");
            return 1;
        }
    } else {
        fprintf(output_stream, "✅ No port conflicts detected\n");
    }
    
    // Initialize process management
    if (process_manager_init() != 0) {
        fprintf(stderr, "❌ Failed to initialize process management\n");
        return 1;
    }
    
    // RKLLM library is statically linked - no dynamic loading needed
    fprintf(output_stream, "✅ RKLLM library available (statically linked)\n");
    
    // Initialize server
    fprintf(output_stream, "⚙️  Initializing MCP Server...\n");
    if (mcp_server_init(&g_server, &config) != 0) {
        fprintf(stderr, "❌ Failed to initialize MCP server\n");
        process_manager_cleanup();
        return 1;
    }
    
    // Print enabled transports (to stderr if STDIO transport is enabled)
    fprintf(output_stream, "📡 Enabled transports:\n");
    if (config.enable_stdio) fprintf(output_stream, "   • STDIO (stdin/stdout)\n");
    if (config.enable_tcp) fprintf(output_stream, "   • TCP (port %d)\n", config.tcp_port);
    if (config.enable_udp) fprintf(output_stream, "   • UDP (port %d)\n", config.udp_port);
    if (config.enable_http) fprintf(output_stream, "   • HTTP (port %d, path %s)\n", config.http_port, config.http_path);
    if (config.enable_websocket) fprintf(output_stream, "   • WebSocket (port %d, path %s)\n", config.ws_port, config.ws_path);
    
    // Start server
    fprintf(output_stream, "🚀 Starting MCP Server...\n");
    if (mcp_server_start(&g_server) != 0) {
        fprintf(stderr, "❌ Failed to start MCP server\n");
        mcp_server_shutdown(&g_server);
        process_manager_cleanup();
        return 1;
    }
    
    fprintf(output_stream, "✅ MCP Server started successfully\n");
    fprintf(output_stream, "🔄 Server running... (Press Ctrl+C to stop)\n");
    fprintf(output_stream, "📊 Status: %s\n", mcp_server_get_status(&g_server));
    
    // Run the main event loop to process requests
    mcp_server_run_event_loop(&g_server);
    
    // Print final statistics
    uint64_t requests, responses, errors, uptime;
    mcp_server_get_stats(&g_server, &requests, &responses, &errors, &uptime);
    
    fprintf(output_stream, "\n📊 Final Statistics:\n");
    fprintf(output_stream, "   • Requests processed: %lu\n", requests);
    fprintf(output_stream, "   • Responses sent: %lu\n", responses);
    fprintf(output_stream, "   • Errors handled: %lu\n", errors);
    fprintf(output_stream, "   • Uptime: %lu seconds\n", uptime);
    
    fprintf(output_stream, "🛑 Shutting down MCP Server...\n");
    mcp_server_shutdown(&g_server);
    process_manager_cleanup();
    fprintf(output_stream, "✅ MCP Server shutdown complete\n");
    
    return 0;
}