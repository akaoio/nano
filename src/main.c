#include "include/mcp/server.h"
#include "common/core.h"
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
    printf("  --log-file FILE      Log to file instead of stderr\n");
    printf("\nDefault configuration:\n");
    printf("STDIO: enabled\n");
    printf("TCP: port 8080\n");
    printf("UDP: port 8081\n");
    printf("HTTP: port 8082\n");
    printf("WebSocket: port 8083\n");
}

int main(int argc, char* argv[]) {
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
        }
    }
    
    // Initialize server
    printf("⚙️  Initializing MCP Server...\n");
    if (mcp_server_init(&g_server, &config) != 0) {
        fprintf(stderr, "❌ Failed to initialize MCP server\n");
        return 1;
    }
    
    // Print enabled transports
    printf("📡 Enabled transports:\n");
    if (config.enable_stdio) printf("   • STDIO (stdin/stdout)\n");
    if (config.enable_tcp) printf("   • TCP (port %d)\n", config.tcp_port);
    if (config.enable_udp) printf("   • UDP (port %d)\n", config.udp_port);
    if (config.enable_http) printf("   • HTTP (port %d, path %s)\n", config.http_port, config.http_path);
    if (config.enable_websocket) printf("   • WebSocket (port %d, path %s)\n", config.ws_port, config.ws_path);
    
    // Start server
    printf("🚀 Starting MCP Server...\n");
    if (mcp_server_start(&g_server) != 0) {
        fprintf(stderr, "❌ Failed to start MCP server\n");
        mcp_server_shutdown(&g_server);
        return 1;
    }
    
    printf("✅ MCP Server started successfully\n");
    printf("🔄 Server running... (Press Ctrl+C to stop)\n");
    printf("📊 Status: %s\n", mcp_server_get_status(&g_server));
    
    // Main server loop
    while (g_running) {
        // In a real implementation, this would handle incoming requests
        // For now, just sleep and let signal handler stop the server
        sleep(1);
        
        // Update uptime
        g_server.uptime_seconds++;
    }
    
    // Print final statistics
    uint64_t requests, responses, errors, uptime;
    mcp_server_get_stats(&g_server, &requests, &responses, &errors, &uptime);
    
    printf("\n📊 Final Statistics:\n");
    printf("   • Requests processed: %lu\n", requests);
    printf("   • Responses sent: %lu\n", responses);
    printf("   • Errors handled: %lu\n", errors);
    printf("   • Uptime: %lu seconds\n", uptime);
    
    printf("🛑 Shutting down MCP Server...\n");
    mcp_server_shutdown(&g_server);
    printf("✅ MCP Server shutdown complete\n");
    
    return 0;
}