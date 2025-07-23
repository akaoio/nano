#ifndef GET_SERVER_CONFIG_H
#define GET_SERVER_CONFIG_H

/**
 * Server configuration structure
 */
typedef struct {
    char* socket_path;          // Unix domain socket path
    int max_connections;        // Maximum concurrent connections
    int listen_backlog;        // Socket listen backlog
    int epoll_max_events;      // Maximum epoll events per wait
    int epoll_timeout_ms;      // Epoll timeout in milliseconds
    int buffer_size;           // I/O buffer size
    int log_level;             // Logging level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
    int connection_buffer_size; // Connection buffer size
    int error_buffer_size;     // Error message buffer size
    int small_error_buffer_size; // Small error buffer size
    int timestamp_buffer_size; // Timestamp buffer size
    int max_path_length;       // Maximum path length
    int method_name_length;    // Maximum method name length
    int init_timeout;          // Initialization timeout
    int async_timeout;         // Async operation timeout
} ServerConfig;

/**
 * Gets server configuration from environment variables and defaults
 * @return Pointer to ServerConfig structure (caller must free)
 */
ServerConfig* get_server_config(void);

/**
 * Frees server configuration structure
 * @param config Configuration structure to free
 */
void free_server_config(ServerConfig* config);

#endif