#include "nano/core/nano/nano.h"
#include "nano/transport/stdio_transport/stdio_transport.h"
#include "nano/transport/tcp_transport/tcp_transport.h"
#include "nano/transport/udp_transport/udp_transport.h"
#include "nano/transport/http_transport/http_transport.h"
#include "nano/transport/ws_transport/ws_transport.h"
#include "common/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static bool g_running = true;

void signal_handler(int sig) {
    (void)sig;
    if (g_running) {
        g_running = false;
        printf("\nShutting down nano...\n");
        nano_stop();
    }
}

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help           Show this help message\n");
    printf("  -t, --tcp PORT       Set TCP transport port (default: 8080)\n");
    printf("  -u, --udp PORT       Set UDP transport port (default: 8081)\n");
    printf("  -w, --ws PORT        Set WebSocket transport port (default: 8082)\n");
    printf("  -H, --http PORT      Set HTTP transport port (default: 8083)\n");
    printf("  --host HOST          Set host for network transports (default: localhost)\n");
    printf("\nAll transports are enabled by default.\n");
    printf("STDIO: always enabled\n");
    printf("TCP: localhost:8080\n");
    printf("UDP: localhost:8081\n");
    printf("WebSocket: localhost:8082\n");
    printf("HTTP: localhost:8083\n");
}

int main(int argc, char* argv[]) {
    printf("🚀 NANO - Neural API Network Orchestrator\n");
    printf("==========================================\n");
    
    // Install signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize nano core
    if (nano_init() != 0) {
        fprintf(stderr, "Failed to initialize nano core\n");
        return 1;
    }
    
    // Parse command line arguments
    bool stdio_enabled = true;   // Enable by default
    bool tcp_enabled = true;     // Enable by default
    bool udp_enabled = true;     // Enable by default
    bool ws_enabled = true;      // Enable by default
    bool http_enabled = true;    // Enable by default
    int tcp_port = 8080;
    int udp_port = 8081;
    int ws_port = 8082;
    int http_port = 8083;
    char* host = "localhost";
    
    // All transports enabled by default
    // Command line arguments can override ports and host
    
    // Parse arguments (override defaults)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tcp") == 0) {
            if (i + 1 < argc) {
                tcp_port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--udp") == 0) {
            if (i + 1 < argc) {
                udp_port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--ws") == 0) {
            if (i + 1 < argc) {
                ws_port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--http") == 0) {
            if (i + 1 < argc) {
                http_port = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "--host") == 0) {
            if (i + 1 < argc) {
                host = argv[++i];
            }
        }
    }
    
    // Setup transports
    if (stdio_enabled) {
        printf("📡 Enabling STDIO transport\n");
        if (nano_add_transport(stdio_transport_get_interface(), nullptr) != 0) {
            fprintf(stderr, "Failed to add STDIO transport\n");
        }
    }
    
    if (tcp_enabled) {
        printf("📡 Enabling TCP transport on port %d\n", tcp_port);
        tcp_transport_config_t tcp_config = {
            .host = host,
            .port = tcp_port,
            .is_server = true
        };
        if (nano_add_transport(tcp_transport_get_interface(), &tcp_config) != 0) {
            fprintf(stderr, "Failed to add TCP transport\n");
        }
    }
    
    if (udp_enabled) {
        printf("📡 Enabling UDP transport on port %d\n", udp_port);
        udp_transport_config_t udp_config = {
            .host = host,
            .port = udp_port
        };
        if (nano_add_transport(udp_transport_get_interface(), &udp_config) != 0) {
            fprintf(stderr, "Failed to add UDP transport\n");
        }
    }
    
    if (ws_enabled) {
        printf("📡 Enabling WebSocket transport on port %d\n", ws_port);
        ws_transport_config_t ws_config = {
            .host = host,
            .port = ws_port,
            .path = "/"
        };
        if (nano_add_transport(ws_transport_get_interface(), &ws_config) != 0) {
            fprintf(stderr, "Failed to add WebSocket transport\n");
        }
    }
    
    if (http_enabled) {
        printf("📡 Enabling HTTP transport on port %d\n", http_port);
        http_transport_config_t http_config = {
            .host = host,
            .port = http_port,
            .path = "/rpc"
        };
        if (nano_add_transport(http_transport_get_interface(), &http_config) != 0) {
            fprintf(stderr, "Failed to add HTTP transport\n");
        }
    }
    
    printf("✅ Nano initialized successfully\n");
    printf("🔄 Running... (Press Ctrl+C to stop)\n");
    
    // Run nano
    int run_result = nano_run();
    if (run_result != 0) {
        fprintf(stderr, "⚠️  Nano run failed with code %d\n", run_result);
    }
    
    printf("🛑 Shutting down nano...\n");
    nano_shutdown();
    printf("✅ Nano shutdown complete\n");
    
    return 0;
}
