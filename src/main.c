#include "include/mcp/server.h"
#include "common/core.h"
#include "lib/core/process_manager.h"
#include "lib/core/settings.h"
#include "lib/core/settings_global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static mcp_server_t g_server = {0};
static mcp_settings_t g_settings = {0};
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
    printf("  -c, --config FILE    Load configuration from JSON file (default: settings.json)\n");
    printf("  -t, --tcp PORT       Override TCP transport port\n");
    printf("  -u, --udp PORT       Override UDP transport port\n");
    printf("  -w, --ws PORT        Override WebSocket transport port\n");
    printf("  -H, --http PORT      Override HTTP transport port\n");
    printf("  --disable-stdio      Disable STDIO transport\n");
    printf("  --disable-tcp        Disable TCP transport\n");
    printf("  --disable-udp        Disable UDP transport\n");
    printf("  --disable-http       Disable HTTP transport\n");
    printf("  --disable-ws         Disable WebSocket transport\n");
    printf("  --force              Kill existing processes using our ports\n");
    printf("  --log-file FILE      Override log file path\n");
    printf("  --generate-config    Generate default settings.json file and exit\n");
    printf("\nConfiguration:\n");
    printf("  Configuration is loaded from settings.json by default.\n");
    printf("  Command line options override settings from the file.\n");
    printf("  Use --generate-config to create a template settings.json file.\n");
}

// Convert settings to legacy config format
static void settings_to_config(const mcp_settings_t* settings, mcp_server_config_t* config) {
    memset(config, 0, sizeof(mcp_server_config_t));
    
    config->enable_stdio = settings->transports.enable_stdio;
    config->enable_tcp = settings->transports.enable_tcp;
    config->enable_udp = settings->transports.enable_udp;
    config->enable_http = settings->transports.enable_http;
    config->enable_websocket = settings->transports.enable_websocket;
    
    config->tcp_port = settings->transports.tcp.port;
    config->udp_port = settings->transports.udp.port;
    config->http_port = settings->transports.http.port;
    config->ws_port = settings->transports.websocket.port;
    
    config->http_path = settings->transports.http.path;
    config->ws_path = settings->transports.websocket.path;
    config->server_name = settings->server.name;
    config->enable_streaming = true; // Always enabled for now
    config->enable_logging = settings->server.enable_logging;
    config->log_file = settings->server.log_file;
}

int main(int argc, char* argv[]) {
    printf("🚀 MCP Server - Model Context Protocol Server\n");
    printf("====================================================\n");
    
    // Install signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    const char* config_file = "settings.json";
    bool generate_config = false;
    
    // Parse command line arguments for config file first
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--generate-config") == 0) {
            generate_config = true;
        } else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) && i + 1 < argc) {
            config_file = argv[++i];
        }
    }
    
    // Generate default config if requested
    if (generate_config) {
        printf("📝 Generating default settings.json...\n");
        settings_get_defaults(&g_settings);
        
        if (settings_save_to_file("settings.json", &g_settings) == 0) {
            printf("✅ Created settings.json with default configuration\n");
            printf("💡 Edit settings.json to customize your server configuration\n");
        } else {
            printf("❌ Failed to create settings.json\n");
            return 1;
        }
        
        settings_free(&g_settings);
        return 0;
    }
    
    // Load settings from file
    printf("📋 Loading configuration from %s...\n", config_file);
    if (settings_load_from_file(config_file, &g_settings) != 0) {
        fprintf(stderr, "❌ Failed to load settings\n");
        return 1;
    }
    
    // Apply command line overrides
    if (settings_apply_overrides(&g_settings, argc, argv) != 0) {
        fprintf(stderr, "❌ Failed to apply command line overrides\n");
        settings_free(&g_settings);
        return 1;
    }
    
    // Validate settings
    if (settings_validate(&g_settings) != 0) {
        fprintf(stderr, "❌ Invalid configuration\n");
        settings_free(&g_settings);
        return 1;
    }
    
    // Initialize global settings access
    if (settings_global_init(&g_settings) != 0) {
        fprintf(stderr, "❌ Failed to initialize global settings\n");
        settings_free(&g_settings);
        return 1;
    }
    
    printf("✅ Configuration loaded successfully\n");
    
    // Print active configuration
    printf("🔧 Active Configuration:\n");
    printf("   Server: %s v%s\n", g_settings.server.name, g_settings.server.version);
    printf("   Logging: %s", g_settings.server.enable_logging ? "enabled" : "disabled");
    if (g_settings.server.log_file) {
        printf(" (file: %s)", g_settings.server.log_file);
    }
    printf("\n");
    
    FILE* output_stream = g_settings.transports.enable_stdio ? stderr : stdout;
    
    // Check for existing instances
    fprintf(output_stream, "🔍 Checking for existing server instances...\n");
    process_status_t status = process_manager_check_existing();
    
    if (status.is_running) {
        fprintf(output_stream, "⚠️  Found running instance: %s (PID %d)\n", status.process_name, status.pid);
        
        if (g_settings.server.force_kill_existing) {
            fprintf(output_stream, "💀 Force kill enabled, terminating existing instance...\n");
            if (process_manager_kill_process(status.pid, true) != 0) {
                fprintf(stderr, "❌ Failed to kill existing instance\n");
                settings_free(&g_settings);
                return 1;
            }
        } else {
            fprintf(stderr, "❌ Server already running (PID %d). Use --force to kill it.\n", status.pid);
            settings_free(&g_settings);
            return 1;
        }
    }
    
    // Scan for port conflicts
    process_port_scan_t ports[] = {
        {g_settings.transports.tcp.port, "TCP", g_settings.transports.enable_tcp},
        {g_settings.transports.udp.port, "UDP", g_settings.transports.enable_udp},
        {g_settings.transports.http.port, "HTTP", g_settings.transports.enable_http},
        {g_settings.transports.websocket.port, "WebSocket", g_settings.transports.enable_websocket}
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
        
        if (g_settings.server.force_kill_existing) {
            fprintf(output_stream, "💀 Force kill enabled, killing conflicting processes...\n");
            int killed = process_manager_kill_conflicts(conflicts, conflict_count, true);
            fprintf(output_stream, "✅ Killed %d of %d conflicting processes\n", killed, conflict_count);
        } else {
            fprintf(stderr, "❌ Port conflicts detected. Use --force to kill conflicting processes.\n");
            settings_free(&g_settings);
            return 1;
        }
    } else {
        fprintf(output_stream, "✅ No port conflicts detected\n");
    }
    
    // Initialize process management
    if (process_manager_init() != 0) {
        fprintf(stderr, "❌ Failed to initialize process management\n");
        settings_free(&g_settings);
        return 1;
    }
    
    fprintf(output_stream, "✅ RKLLM library available (statically linked)\n");
    
    // Convert settings to legacy config format
    mcp_server_config_t config;
    settings_to_config(&g_settings, &config);
    
    // Initialize server
    fprintf(output_stream, "⚙️  Initializing MCP Server...\n");
    if (mcp_server_init(&g_server, &config) != 0) {
        fprintf(stderr, "❌ Failed to initialize MCP server\n");
        process_manager_cleanup();
        settings_free(&g_settings);
        return 1;
    }
    
    // Print enabled transports
    fprintf(output_stream, "📡 Enabled transports:\n");
    if (g_settings.transports.enable_stdio) 
        fprintf(output_stream, "   • STDIO (stdin/stdout)\n");
    if (g_settings.transports.enable_tcp) 
        fprintf(output_stream, "   • TCP (%s:%d)\n", g_settings.transports.tcp.host, g_settings.transports.tcp.port);
    if (g_settings.transports.enable_udp) 
        fprintf(output_stream, "   • UDP (%s:%d)\n", g_settings.transports.udp.host, g_settings.transports.udp.port);
    if (g_settings.transports.enable_http) 
        fprintf(output_stream, "   • HTTP (%s:%d%s)\n", g_settings.transports.http.host, g_settings.transports.http.port, g_settings.transports.http.path);
    if (g_settings.transports.enable_websocket) 
        fprintf(output_stream, "   • WebSocket (%s:%d%s)\n", g_settings.transports.websocket.host, g_settings.transports.websocket.port, g_settings.transports.websocket.path);
    
    // Print RKLLM settings
    fprintf(output_stream, "🤖 RKLLM Configuration:\n");
    fprintf(output_stream, "   • Default model: %s\n", g_settings.rkllm.default_model_path);
    fprintf(output_stream, "   • Max context: %d tokens\n", g_settings.rkllm.max_context_len);
    fprintf(output_stream, "   • Max new tokens: %d\n", g_settings.rkllm.max_new_tokens);
    fprintf(output_stream, "   • Temperature: %.2f\n", g_settings.rkllm.temperature);
    fprintf(output_stream, "   • CPU mask: 0x%X (%d CPUs)\n", g_settings.rkllm.extend.enabled_cpus_mask, g_settings.rkllm.extend.enabled_cpus_num);
    
    // Print buffer settings
    fprintf(output_stream, "📊 Buffer Configuration:\n");
    fprintf(output_stream, "   • Request buffer: %zu bytes\n", g_settings.buffers.request_buffer_size);
    fprintf(output_stream, "   • Response buffer: %zu bytes\n", g_settings.buffers.response_buffer_size);
    fprintf(output_stream, "   • Max JSON size: %zu bytes\n", g_settings.buffers.max_json_size);
    
    // Start server
    fprintf(output_stream, "🚀 Starting MCP Server...\n");
    if (mcp_server_start(&g_server) != 0) {
        fprintf(stderr, "❌ Failed to start MCP server\n");
        mcp_server_shutdown(&g_server);
        process_manager_cleanup();
        settings_free(&g_settings);
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
    settings_global_shutdown();
    settings_free(&g_settings);
    fprintf(output_stream, "✅ MCP Server shutdown complete\n");
    
    return 0;
}