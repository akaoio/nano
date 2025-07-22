#define _DEFAULT_SOURCE
#include "websocket.h"
#include "../core/settings_global.h"
#include "../protocol/adapter.h"
#include "../core/stream_manager.h"
#include "common/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <errno.h>
#include <ws.h>

// Global state for backward compatibility
static ws_transport_config_t g_config = {0};
static pthread_t g_server_thread;
static volatile bool g_server_running = false;
static pthread_mutex_t g_message_mutex = PTHREAD_MUTEX_INITIALIZER;
static char* g_received_message = NULL;
static size_t g_message_buffer_size = 0;
static volatile bool g_message_ready = false;
static ws_cli_conn_t g_current_client = 0;

// New multi-client WebSocket transport instance
static websocket_transport_t g_websocket_transport = {0};

// Forward declarations
static int ws_transport_base_init(transport_base_t* base, void* config);
static void ws_transport_base_shutdown(transport_base_t* base);
static int ws_transport_base_connect(transport_base_t* base);
static int ws_transport_base_disconnect(transport_base_t* base);
static int ws_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int ws_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int ws_transport_base_is_connected(transport_base_t* base);

// WebSocket callback functions for multi-client server
void websocket_on_connect(ws_cli_conn_t connection) {
    websocket_transport_t* transport = &g_websocket_transport;
    
    printf("WebSocket: Client connected from %s:%s\n", 
           ws_getaddress(connection), ws_getport(connection));
    
    pthread_mutex_lock(&transport->connections_mutex);
    
    // Find available connection slot
    for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (transport->connections[i] == 0) {
            transport->connections[i] = connection;
            transport->connection_count++;
            printf("WebSocket: Registered connection (total: %zu)\n", transport->connection_count);
            break;
        }
    }
    
    if (transport->connection_count >= MAX_WS_CONNECTIONS) {
        printf("WebSocket: Maximum connections reached (%d), but connection accepted\n", MAX_WS_CONNECTIONS);
    }
    
    pthread_mutex_unlock(&transport->connections_mutex);
}

void websocket_on_disconnect(ws_cli_conn_t connection) {
    websocket_transport_t* transport = &g_websocket_transport;
    
    printf("WebSocket: Client disconnected %s\n", ws_getaddress(connection));
    
    pthread_mutex_lock(&transport->connections_mutex);
    
    // Remove connection from active list
    for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (transport->connections[i] == connection) {
            transport->connections[i] = 0;
            if (transport->connection_count > 0) {
                transport->connection_count--;
            }
            printf("WebSocket: Removed connection (remaining: %zu)\n", transport->connection_count);
            break;
        }
    }
    
    pthread_mutex_unlock(&transport->connections_mutex);
}

void websocket_on_message(ws_cli_conn_t connection, const unsigned char *msg, uint64_t size, int type) {
    websocket_transport_t* transport = &g_websocket_transport;
    
    if (size >= WS_RESPONSE_SIZE) {
        printf("WebSocket: Message too large (%lu bytes), truncating\n", size);
        size = WS_RESPONSE_SIZE - 1;
    }
    
    printf("WebSocket: Received %lu bytes from %s: %.100s%s\n", 
           size, ws_getaddress(connection), (char*)msg, size > 100 ? "..." : "");
    
    // Process message through MCP adapter
    char response_buffer[WS_RESPONSE_SIZE];
    size_t response_size = sizeof(response_buffer);
    // For now, use simple echo response until proper MCP adapter integration
    snprintf(response_buffer, sizeof(response_buffer), 
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{\"status\":\"received\",\"bytes\":%lu}}", 
            size);
    response_size = strlen(response_buffer);
    int result = 0; // Success
    
    if (result == 0 && response_size > 0) {
        // Send response back to the client that sent the message
        if (ws_sendframe_txt(connection, response_buffer) == 0) {
            printf("WebSocket: Sent %zu bytes to %s\n", response_size, ws_getaddress(connection));
        } else {
            printf("WebSocket: Failed to send response to %s\n", ws_getaddress(connection));
        }
    } else {
        // Send error response
        const char* error_response = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32603,\"message\":\"Internal error\"},\"id\":null}";
        if (ws_sendframe_txt(connection, error_response) != 0) {
            printf("WebSocket: Failed to send error response to %s\n", ws_getaddress(connection));
        }
    }
    
    // Also store for legacy recv function
    pthread_mutex_lock(&transport->message_mutex);
    if (size < transport->message_buffer_size) {
        memcpy(transport->message_buffer, msg, size);
        transport->message_buffer[size] = '\0';
        transport->message_ready = true;
    }
    pthread_mutex_unlock(&transport->message_mutex);
}

// WebSocket server thread for multi-client support
void* websocket_server_thread(void* arg) {
    websocket_transport_t* transport = (websocket_transport_t*)arg;
    
    printf("WebSocket: Starting server on port %d\n", transport->port);
    
    // Start wsServer - this call blocks
    ws_socket(&(struct ws_server){
        .host = transport->host ? transport->host : "0.0.0.0",
        .port = transport->port,
        .thread_loop = 0,
        .timeout_ms = 1000,
        .evs.onopen = &websocket_on_connect,
        .evs.onclose = &websocket_on_disconnect,
        .evs.onmessage = &websocket_on_message
    });
    
    return NULL;
}

// Start WebSocket server
int websocket_server_start(websocket_transport_t* transport) {
    if (!transport || transport->running) {
        return -1;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&transport->connections_mutex, NULL) != 0) {
        printf("WebSocket: Failed to initialize connections mutex: %s\n", strerror(errno));
        return -1;
    }
    
    if (pthread_mutex_init(&transport->message_mutex, NULL) != 0) {
        printf("WebSocket: Failed to initialize message mutex: %s\n", strerror(errno));
        pthread_mutex_destroy(&transport->connections_mutex);
        return -1;
    }
    
    transport->running = true;
    transport->connection_count = 0;
    
    // Initialize all connection slots
    for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
        transport->connections[i] = 0;
    }
    
    // Initialize message buffer
    transport->message_buffer_size = WS_RESPONSE_SIZE;
    transport->message_buffer = calloc(1, transport->message_buffer_size);
    if (!transport->message_buffer) {
        printf("WebSocket: Failed to allocate message buffer\n");
        pthread_mutex_destroy(&transport->connections_mutex);
        pthread_mutex_destroy(&transport->message_mutex);
        return -1;
    }
    
    // Create server thread
    int result = pthread_create(&transport->server_thread, NULL, websocket_server_thread, transport);
    if (result != 0) {
        printf("WebSocket: Failed to create server thread: %s\n", strerror(result));
        free(transport->message_buffer);
        transport->message_buffer = NULL;
        transport->running = false;
        pthread_mutex_destroy(&transport->connections_mutex);
        pthread_mutex_destroy(&transport->message_mutex);
        return -1;
    }
    
    // Give server thread time to start
    usleep(100000); // 100ms
    
    transport->connected = true;
    return 0;
}

// Stop WebSocket server
int websocket_server_stop(websocket_transport_t* transport) {
    if (!transport || !transport->running) {
        return 0;
    }
    
    printf("WebSocket: Stopping server...\n");
    transport->running = false;
    
    // Close all client connections
    pthread_mutex_lock(&transport->connections_mutex);
    for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (transport->connections[i] != 0) {
            ws_close_client(transport->connections[i]);
            transport->connections[i] = 0;
        }
    }
    transport->connection_count = 0;
    pthread_mutex_unlock(&transport->connections_mutex);
    
    // Note: We can't cleanly stop the wsServer thread as it doesn't provide
    // a clean shutdown mechanism, so we'll let it terminate naturally
    pthread_detach(transport->server_thread);
    
    // Cleanup resources
    if (transport->message_buffer) {
        free(transport->message_buffer);
        transport->message_buffer = NULL;
    }
    
    pthread_mutex_destroy(&transport->connections_mutex);
    pthread_mutex_destroy(&transport->message_mutex);
    transport->connected = false;
    
    printf("WebSocket: Server stopped\n");
    return 0;
}

// Send data to all connected clients
int websocket_send_to_all_clients(websocket_transport_t* transport, const char* data, size_t len) {
    if (!transport || !data || len == 0) {
        return -1;
    }
    
    int clients_sent = 0;
    
    pthread_mutex_lock(&transport->connections_mutex);
    for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (transport->connections[i] != 0) {
            if (ws_sendframe_txt(transport->connections[i], data) == 0) {
                clients_sent++;
            } else {
                printf("WebSocket: Failed to send to client %s\n", 
                       ws_getaddress(transport->connections[i]));
            }
        }
    }
    pthread_mutex_unlock(&transport->connections_mutex);
    
    return clients_sent;
}

// Streaming support - send chunk to all clients
int websocket_send_stream_chunk(transport_base_t* base, const mcp_stream_chunk_t* chunk) {
    websocket_transport_t* transport = (websocket_transport_t*)base;
    if (!transport || !chunk) {
        return -1;
    }
    
    // Format chunk as JSON-RPC response
    char chunk_json[4096];
    int result = mcp_adapter_format_stream_chunk(chunk, chunk_json, sizeof(chunk_json));
    if (result != MCP_ADAPTER_OK) {
        printf("WebSocket: Failed to format stream chunk\n");
        return -1;
    }
    
    // Send to all connected clients
    int clients_sent = websocket_send_to_all_clients(transport, chunk_json, strlen(chunk_json));
    printf("WebSocket: Sent stream chunk to %d clients\n", clients_sent);
    
    return clients_sent > 0 ? 0 : -1;
}

// Get connection count
int websocket_get_connection_count(transport_base_t* base) {
    websocket_transport_t* transport = (websocket_transport_t*)base;
    if (!transport) {
        return -1;
    }
    
    pthread_mutex_lock(&transport->connections_mutex);
    size_t count = transport->connection_count;
    pthread_mutex_unlock(&transport->connections_mutex);
    
    return (int)count;
}

// Test connection drop simulation
bool websocket_test_connection_drop(void) {
    websocket_transport_t* transport = &g_websocket_transport;
    
    if (!transport->running || transport->connection_count == 0) {
        return false; // No connections to drop
    }
    
    // Find first active client and close its connection
    pthread_mutex_lock(&transport->connections_mutex);
    for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (transport->connections[i] != 0) {
            printf("WebSocket: Simulating connection drop for client %s\n", 
                   ws_getaddress(transport->connections[i]));
            ws_close_client(transport->connections[i]);
            transport->connections[i] = 0;
            transport->connection_count--;
            pthread_mutex_unlock(&transport->connections_mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&transport->connections_mutex);
    
    return false;
}

// Create new WebSocket transport instance
websocket_transport_t* websocket_transport_create(void) {
    websocket_transport_t* transport = malloc(sizeof(websocket_transport_t));
    if (!transport) {
        return NULL;
    }
    
    memset(transport, 0, sizeof(websocket_transport_t));
    
    // Initialize base transport interface
    strncpy(transport->base.name, "websocket", sizeof(transport->base.name) - 1);
    transport->base.name[sizeof(transport->base.name) - 1] = '\0';
    transport->base.state = TRANSPORT_STATE_DISCONNECTED;
    transport->base.initialized = false;
    transport->base.running = false;
    transport->base.init = ws_transport_base_init;
    transport->base.shutdown = ws_transport_base_shutdown;
    transport->base.connect = ws_transport_base_connect;
    transport->base.disconnect = ws_transport_base_disconnect;
    transport->base.send = ws_transport_base_send;
    transport->base.recv = ws_transport_base_recv;
    transport->base.is_connected = ws_transport_base_is_connected;
    transport->base.send_stream_chunk = (int (*)(struct transport_base*, const void*))websocket_send_stream_chunk;
    transport->base.get_connection_count = websocket_get_connection_count;
    transport->base.impl_data = transport;
    
    return transport;
}

// Legacy wsServer callback functions for backward compatibility
void on_ws_open(ws_cli_conn_t client) {
    printf("WebSocket client connected: %s:%s\n", 
           ws_getaddress(client), ws_getport(client));
    g_current_client = client;
}

void on_ws_close(ws_cli_conn_t client) {
    printf("WebSocket client disconnected: %s\n", ws_getaddress(client));
    if (g_current_client == client) {
        g_current_client = 0;
    }
}

void on_ws_message(ws_cli_conn_t client, const unsigned char *msg, uint64_t size, int type) {
    pthread_mutex_lock(&g_message_mutex);
    
    // Store the received message
    if (size < g_message_buffer_size) {
        memcpy(g_received_message, msg, size);
        g_received_message[size] = '\0';
        g_current_client = client;
        g_message_ready = true;
    }
    
    pthread_mutex_unlock(&g_message_mutex);
}

// Legacy WebSocket server thread function
void* ws_server_thread_func(void* arg) {
    ws_transport_config_t* config = (ws_transport_config_t*)arg;
    
    printf("✅ WebSocket legacy server listening on port %d\n", config->port);
    
    // Start wsServer - this call blocks
    ws_socket(&(struct ws_server){
        .host = "0.0.0.0",
        .port = config->port,
        .thread_loop = 0,
        .timeout_ms = 1000,
        .evs.onopen = &on_ws_open,
        .evs.onclose = &on_ws_close,
        .evs.onmessage = &on_ws_message
    });
    
    return NULL;
}

// Legacy functions for backward compatibility
int ws_transport_init(void* config) {
    if (!config) return -1;
    
    ws_transport_config_t* cfg = (ws_transport_config_t*)config;
    g_config = *cfg;
    
    // Copy string fields
    if (cfg->host) {
        g_config.host = strdup(cfg->host);
    }
    if (cfg->path) {
        g_config.path = strdup(cfg->path);
    }
    
    // Initialize message buffer from settings
    g_message_buffer_size = SETTING_BUFFER(websocket_message_buffer_size);
    if (g_message_buffer_size == 0) g_message_buffer_size = 8192; // fallback
    
    g_received_message = calloc(1, g_message_buffer_size);
    if (!g_received_message) {
        return -1;
    }
    
    // WebSocket server defaults
    g_config.connected = false;
    g_message_ready = false;
    g_config.initialized = true;
    g_config.running = true;
    
    // Initialize new transport
    g_websocket_transport.host = g_config.host ? strdup(g_config.host) : NULL;
    g_websocket_transport.port = g_config.port;
    g_websocket_transport.path = g_config.path ? strdup(g_config.path) : NULL;
    g_websocket_transport.initialized = true;
    
    return 0;
}

int ws_transport_connect(void) {
    if (!g_config.initialized) return -1;
    
    if (g_config.connected) return 0; // Already connected
    
    // Start new multi-client server
    g_websocket_transport.host = g_config.host ? strdup(g_config.host) : NULL;
    g_websocket_transport.port = g_config.port;
    g_websocket_transport.path = g_config.path ? strdup(g_config.path) : NULL;
    
    if (websocket_server_start(&g_websocket_transport) == 0) {
        printf("✅ WebSocket multi-client server started on port %d\n", g_config.port);
    }
    
    // Start legacy WebSocket server in a separate thread for backward compatibility
    if (pthread_create(&g_server_thread, NULL, ws_server_thread_func, &g_config) != 0) {
        printf("Failed to create WebSocket legacy server thread\n");
        return -1;
    }
    
    // Give the server thread a moment to start
    struct timeval tv = {0, 100000}; // 100ms
    select(0, NULL, NULL, NULL, &tv);
    
    g_server_running = true;
    g_config.connected = true;
    return 0;
}

int ws_transport_disconnect(void) {
    if (!g_config.initialized) return -1;
    
    // Stop multi-client server
    websocket_server_stop(&g_websocket_transport);
    
    // Stop the legacy server thread if running
    if (g_server_running) {
        g_server_running = false;
        
        // Note: We can't cleanly join the thread as wsServer doesn't provide
        // a clean shutdown mechanism, so we'll detach it
        pthread_detach(g_server_thread);
    }
    
    // Close current client if any
    if (g_current_client != 0) {
        ws_close_client(g_current_client);
        g_current_client = 0;
    }
    
    g_config.connected = false;
    return 0;
}

bool ws_transport_is_connected(void) {
    return g_config.initialized && (g_config.connected || g_websocket_transport.connected);
}

int ws_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0) {
        return -1;
    }
    
    // Use new multi-client implementation if available
    if (g_websocket_transport.running) {
        return websocket_send_to_all_clients(&g_websocket_transport, data, len) > 0 ? 0 : -1;
    }
    
    // Legacy single client implementation
    if (!g_config.connected) {
        return -1;
    }
    
    pthread_mutex_lock(&g_message_mutex);
    
    // Send message to current client using wsServer
    if (g_current_client != 0) {
        int result = ws_sendframe_txt(g_current_client, data);
        pthread_mutex_unlock(&g_message_mutex);
        return (result == 0) ? 0 : -1;
    }
    
    pthread_mutex_unlock(&g_message_mutex);
    return -1; // No client to send to
}

int ws_transport_recv_raw(char* buffer, size_t buffer_size, int timeout_ms) {
    if (!g_config.initialized || !buffer || buffer_size == 0 || !g_config.connected) {
        return -1;
    }
    
    // Use new multi-client implementation message buffer
    websocket_transport_t* transport = &g_websocket_transport;
    if (transport->message_buffer && transport->message_ready) {
        pthread_mutex_lock(&transport->message_mutex);
        if (transport->message_ready) {
            size_t msg_len = strlen(transport->message_buffer);
            if (msg_len < buffer_size) {
                strcpy(buffer, transport->message_buffer);
                transport->message_ready = false;
                pthread_mutex_unlock(&transport->message_mutex);
                return 0;
            }
        }
        pthread_mutex_unlock(&transport->message_mutex);
    }
    
    // Legacy implementation
    pthread_mutex_lock(&g_message_mutex);
    
    // Check if we have a message ready
    if (g_message_ready) {
        // Copy the message to the buffer
        size_t msg_len = strlen(g_received_message);
        if (msg_len < buffer_size) {
            strcpy(buffer, g_received_message);
            g_message_ready = false; // Mark as consumed
            pthread_mutex_unlock(&g_message_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_message_mutex);
    
    // If no message is ready, wait for timeout_ms
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    // Use select to provide timeout behavior
    select(0, NULL, NULL, NULL, &timeout);
    
    // Check again after timeout
    pthread_mutex_lock(&g_message_mutex);
    if (g_message_ready) {
        size_t msg_len = strlen(g_received_message);
        if (msg_len < buffer_size) {
            strcpy(buffer, g_received_message);
            g_message_ready = false; // Mark as consumed
            pthread_mutex_unlock(&g_message_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&g_message_mutex);
    
    return -1; // No message received within timeout
}

void ws_transport_shutdown(void) {
    ws_transport_disconnect();
    
    if (g_config.host) {
        free(g_config.host);
        g_config.host = NULL;
    }
    
    if (g_config.path) {
        free(g_config.path);
        g_config.path = NULL;
    }
    
    if (g_websocket_transport.host) {
        free(g_websocket_transport.host);
        g_websocket_transport.host = NULL;
    }
    
    if (g_websocket_transport.path) {
        free(g_websocket_transport.path);
        g_websocket_transport.path = NULL;
    }
    
    // Free message buffer
    free(g_received_message);
    g_received_message = NULL;
    g_message_buffer_size = 0;
    
    g_config.running = false;
    g_config.initialized = false;
    g_websocket_transport.initialized = false;
}

// Transport base interface implementations
static int ws_transport_base_init(transport_base_t* base, void* config) {
    return ws_transport_init(config);
}

static void ws_transport_base_shutdown(transport_base_t* base) {
    ws_transport_shutdown();
}

static int ws_transport_base_connect(transport_base_t* base) {
    return ws_transport_connect();
}

static int ws_transport_base_disconnect(transport_base_t* base) {
    return ws_transport_disconnect();
}

static int ws_transport_base_send(transport_base_t* base, const char* data, size_t len) {
    return ws_transport_send_raw(data, len);
}

static int ws_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms) {
    return ws_transport_recv_raw(buffer, buffer_size, timeout_ms);
}

static int ws_transport_base_is_connected(transport_base_t* base) {
    return ws_transport_is_connected() ? 1 : 0;
}

static transport_base_t g_ws_transport_base = {
    .name = "websocket",
    .state = TRANSPORT_STATE_DISCONNECTED,
    .initialized = false,
    .running = false,
    .init = ws_transport_base_init,
    .shutdown = ws_transport_base_shutdown,
    .connect = ws_transport_base_connect,
    .disconnect = ws_transport_base_disconnect,
    .send = ws_transport_base_send,
    .recv = ws_transport_base_recv,
    .is_connected = ws_transport_base_is_connected,
    .send_stream_chunk = (int (*)(struct transport_base*, const void*))websocket_send_stream_chunk,
    .get_connection_count = websocket_get_connection_count,
    .impl_data = &g_websocket_transport,
    .on_error = NULL,
    .on_state_change = NULL
};

transport_base_t* ws_transport_get_interface(void) {
    return &g_ws_transport_base;
}