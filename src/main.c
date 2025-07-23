#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

// Server functions
#include "server/create_socket/create_socket.h"
#include "server/bind_socket/bind_socket.h"
#include "server/listen_socket/listen_socket.h"
#include "server/accept_connection/accept_connection.h"
#include "server/setup_epoll/setup_epoll.h"
#include "server/cleanup_socket/cleanup_socket.h"
#include "server/install_signal_handlers/install_signal_handlers.h"
#include "server/check_shutdown_requested/check_shutdown_requested.h"

// Connection management
#include "connection/create_connection/create_connection.h"
#include "connection/add_connection/add_connection.h"
#include "connection/find_connection/find_connection.h"
#include "connection/remove_connection/remove_connection.h"

// JSON-RPC processing
#include "jsonrpc/parse_request/parse_request.h"
#include "jsonrpc/handle_request/handle_request.h"

// Utility functions
#include "utils/log_message/log_message.h"

// Global variables for cleanup
static int server_socket = -1;
static int epoll_fd = -1;
static volatile int running = 1;
static ConnectionManager* conn_manager = NULL;
static const char* global_socket_path = NULL;

void cleanup_and_exit(void) {
    if (global_socket_path) {
        unlink(global_socket_path);
        log_message("Removed socket file on exit: %s", global_socket_path);
    }
}

void signal_handler(int signum) {
    running = 0;
    log_message("Received signal %d, shutting down gracefully", signum);
}

int main(int argc, char *argv[]) {
    const char *socket_path = getenv("RKLLM_UDS_PATH");
    if (!socket_path) {
        socket_path = "/tmp/rkllm.sock";
    }
    global_socket_path = socket_path;

    log_message("Starting RKLLM Unix Domain Socket Server with Crash Protection");
    log_message("Socket path: %s", socket_path);

    // Install hardened signal handlers for crash protection
    if (install_signal_handlers() != 0) {
        log_message("Failed to install signal handlers");
        return EXIT_FAILURE;
    }

    // Install cleanup handlers
    atexit(cleanup_and_exit);

    // Clean up any existing socket file
    if (unlink(socket_path) == 0) {
        log_message("Removed existing socket file: %s", socket_path);
    }

    // Initialize connection manager
    conn_manager = malloc(sizeof(ConnectionManager));
    if (!conn_manager) {
        log_message("Failed to allocate connection manager");
        return EXIT_FAILURE;
    }
    conn_manager->max_connections = 100;
    conn_manager->count = 0;
    conn_manager->connections = calloc(conn_manager->max_connections, sizeof(Connection*));
    if (!conn_manager->connections) {
        log_message("Failed to allocate connections array");
        free(conn_manager);
        return EXIT_FAILURE;
    }

    // Create Unix domain socket
    server_socket = create_socket(socket_path);
    if (server_socket < 0) {
        log_message("Failed to create socket");
        return EXIT_FAILURE;
    }

    // Bind socket to path
    if (bind_socket(server_socket, socket_path) < 0) {
        log_message("Failed to bind socket");
        cleanup_socket(server_socket, socket_path);
        return EXIT_FAILURE;
    }

    // Set socket to listen mode
    if (listen_socket(server_socket, 128) < 0) {
        log_message("Failed to listen on socket");
        cleanup_socket(server_socket, socket_path);
        return EXIT_FAILURE;
    }

    // Setup epoll for non-blocking I/O
    epoll_fd = setup_epoll();
    if (epoll_fd < 0) {
        log_message("Failed to setup epoll");
        cleanup_socket(server_socket, socket_path);
        return EXIT_FAILURE;
    }

    // Add server socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) < 0) {
        log_message("Failed to add server socket to epoll: %s", strerror(errno));
        close(epoll_fd);
        cleanup_socket(server_socket, socket_path);
        return EXIT_FAILURE;
    }

    log_message("Server started successfully, waiting for connections");

    // Main event loop with crash protection
    struct epoll_event events[64];
    while (running && !is_shutdown_requested()) {
        int event_count = epoll_wait(epoll_fd, events, 64, 1000); // 1 second timeout
        
        // Check for shutdown request from signal handler
        if (is_shutdown_requested()) {
            log_message("Shutdown requested by signal handler");
            break;
        }
        
        if (event_count < 0) {
            if (errno == EINTR) {
                continue; // Signal interrupted, check running flag
            }
            log_message("epoll_wait failed: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < event_count; i++) {
            if (events[i].data.fd == server_socket) {
                // New connection request
                int client_fd = accept_connection(server_socket);
                if (client_fd >= 0) {
                    log_message("Accepted new connection: fd=%d", client_fd);
                    
                    // Create connection object
                    Connection* conn = create_connection(client_fd);
                    if (conn && add_connection(conn_manager, conn) == 0) {
                        // Add client to epoll
                        struct epoll_event client_event;
                        client_event.events = EPOLLIN;
                        client_event.data.fd = client_fd;
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) < 0) {
                            log_message("Failed to add client to epoll: %s", strerror(errno));
                            remove_connection(conn_manager, client_fd);
                            close(client_fd);
                        }
                    } else {
                        log_message("Failed to create or add connection");
                        close(client_fd);
                    }
                }
            } else {
                // Client data available
                int client_fd = events[i].data.fd;
                log_message("Data available on client fd=%d", client_fd);
                
                // Find connection
                Connection* conn = find_connection(conn_manager, client_fd);
                if (conn) {
                    // Read data from client
                    char buffer[4096];
                    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0';
                        log_message("Received data: %s", buffer);
                        
                        // Parse JSON-RPC request
                        JSONRPCRequest* req = parse_request(buffer);
                        if (req && req->is_valid) {
                            log_message("Valid JSON-RPC request: method=%s", req->method);
                            // Handle the request
                            handle_request(req, conn);
                        } else {
                            log_message("Invalid JSON-RPC request");
                        }
                        
                        // Clean up request
                        if (req) {
                            if (req->jsonrpc) free(req->jsonrpc);
                            if (req->method) free(req->method);
                            if (req->params) json_object_put(req->params);
                            if (req->id) json_object_put(req->id);
                            free(req);
                        }
                    } else if (bytes_read == 0) {
                        log_message("Client disconnected: fd=%d", client_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        remove_connection(conn_manager, client_fd);
                        close(client_fd);
                    } else {
                        log_message("Read error on fd=%d: %s", client_fd, strerror(errno));
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        remove_connection(conn_manager, client_fd);
                        close(client_fd);
                    }
                } else {
                    log_message("Connection not found for fd=%d", client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                }
            }
        }
    }

    // Cleanup
    log_message("Shutting down server");
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
    cleanup_socket(server_socket, socket_path);
    
    // Explicitly remove socket file
    unlink(socket_path);
    log_message("Socket file removed: %s", socket_path);
    
    // Clean up connection manager
    if (conn_manager) {
        for (int i = 0; i < conn_manager->max_connections; i++) {
            if (conn_manager->connections[i]) {
                close(conn_manager->connections[i]->fd);
                free(conn_manager->connections[i]);
            }
        }
        free(conn_manager->connections);
        free(conn_manager);
    }
    
    log_message("Server shutdown complete");

    return EXIT_SUCCESS;
}