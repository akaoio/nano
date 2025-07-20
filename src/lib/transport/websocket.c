#include "websocket.h"
#include "common/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <ws.h>

static ws_transport_config_t g_config = {0};
static pthread_t g_server_thread;
static volatile bool g_server_running = false;
static pthread_mutex_t g_message_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_received_message[8192] = {0};
static volatile bool g_message_ready = false;
static ws_cli_conn_t g_current_client = 0;

// wsServer callback functions
void on_ws_open(ws_cli_conn_t client) {
    printf("WebSocket client connected: %s:%s\n", 
           ws_getaddress(client), ws_getport(client));
}

void on_ws_close(ws_cli_conn_t client) {
    printf("WebSocket client disconnected: %s\n", ws_getaddress(client));
}

void on_ws_message(ws_cli_conn_t client, const unsigned char *msg, uint64_t size, int type) {
    pthread_mutex_lock(&g_message_mutex);
    
    // Store the received message
    if (size < sizeof(g_received_message)) {
        memcpy(g_received_message, msg, size);
        g_received_message[size] = '\0';
        g_current_client = client;
        g_message_ready = true;
    }
    
    pthread_mutex_unlock(&g_message_mutex);
}

// WebSocket server thread function
void* ws_server_thread_func(void* arg) {
    ws_transport_config_t* config = (ws_transport_config_t*)arg;
    
    printf("✅ WebSocket server listening on port %d\n", config->port);
    
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
    
    // WebSocket server defaults
    g_config.connected = false;
    g_message_ready = false;
    
    g_config.initialized = true;
    g_config.running = true;
    return 0;
}

int ws_transport_connect(void) {
    if (!g_config.initialized) return -1;
    
    if (g_config.connected) return 0; // Already connected
    
    // Start WebSocket server in a separate thread
    if (pthread_create(&g_server_thread, NULL, ws_server_thread_func, &g_config) != 0) {
        printf("Failed to create WebSocket server thread\n");
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
    
    // Stop the server thread if running
    if (g_server_running) {
        g_server_running = false;
        
        // Wait for server thread to finish
        if (pthread_join(g_server_thread, NULL) != 0) {
            printf("Warning: Failed to join WebSocket server thread\n");
        }
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
    return g_config.initialized && g_config.connected && g_server_running;
}





int ws_transport_send_raw(const char* data, size_t len) {
    if (!g_config.initialized || !data || len == 0 || !g_config.connected) {
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
    
    // Poll for messages from wsServer using our message queue
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
    
    g_config.running = false;
    g_config.initialized = false;
}

// Forward declarations for transport_base interface
static int ws_transport_base_init(transport_base_t* base, void* config);
static void ws_transport_base_shutdown(transport_base_t* base);
static int ws_transport_base_connect(transport_base_t* base);
static int ws_transport_base_disconnect(transport_base_t* base);
static int ws_transport_base_send(transport_base_t* base, const char* data, size_t len);
static int ws_transport_base_recv(transport_base_t* base, char* buffer, size_t buffer_size, int timeout_ms);
static int ws_transport_base_is_connected(transport_base_t* base);

static transport_base_t g_ws_transport = {
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
    .impl_data = NULL,
    .on_error = NULL,
    .on_state_change = NULL
};

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

transport_base_t* ws_transport_get_interface(void) {
    return &g_ws_transport;
}