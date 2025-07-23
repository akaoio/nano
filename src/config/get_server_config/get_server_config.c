#include "get_server_config.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Default configuration values
#define DEFAULT_SOCKET_PATH "/tmp/rkllm.sock"
#define DEFAULT_MAX_CONNECTIONS 100
#define DEFAULT_LISTEN_BACKLOG 128
#define DEFAULT_EPOLL_MAX_EVENTS 64
#define DEFAULT_EPOLL_TIMEOUT_MS 1000
#define DEFAULT_BUFFER_SIZE 4096
#define DEFAULT_LOG_LEVEL 1  // INFO level
#define DEFAULT_CONNECTION_BUFFER_SIZE 4096
#define DEFAULT_ERROR_BUFFER_SIZE 1024
#define DEFAULT_SMALL_ERROR_BUFFER_SIZE 256
#define DEFAULT_TIMESTAMP_BUFFER_SIZE 64
#define DEFAULT_MAX_PATH_LENGTH 4096
#define DEFAULT_METHOD_NAME_LENGTH 128
#define DEFAULT_INIT_TIMEOUT 5000
#define DEFAULT_ASYNC_TIMEOUT 3000

/**
 * Gets integer value from environment variable with default fallback
 */
static int get_env_int(const char* env_name, int default_value) {
    const char* env_value = getenv(env_name);
    if (!env_value) {
        return default_value;
    }
    
    char* endptr;
    long value = strtol(env_value, &endptr, 10);
    
    // Check for conversion errors
    if (*endptr != '\0' || value < 0 || value > INT_MAX) {
        return default_value;
    }
    
    return (int)value;
}

/**
 * Gets string value from environment variable with default fallback
 */
static char* get_env_string(const char* env_name, const char* default_value) {
    const char* env_value = getenv(env_name);
    const char* value = env_value ? env_value : default_value;
    
    // Allocate and copy string
    size_t len = strlen(value);
    char* result = malloc(len + 1);
    if (!result) {
        return NULL;
    }
    
    strcpy(result, value);
    return result;
}

ServerConfig* get_server_config(void) {
    ServerConfig* config = malloc(sizeof(ServerConfig));
    if (!config) {
        return NULL;
    }
    
    // Load configuration from environment variables with defaults
    config->socket_path = get_env_string("RKLLM_SOCKET_PATH", DEFAULT_SOCKET_PATH);
    config->max_connections = get_env_int("RKLLM_MAX_CONNECTIONS", DEFAULT_MAX_CONNECTIONS);
    config->listen_backlog = get_env_int("RKLLM_LISTEN_BACKLOG", DEFAULT_LISTEN_BACKLOG);
    config->epoll_max_events = get_env_int("RKLLM_EPOLL_MAX_EVENTS", DEFAULT_EPOLL_MAX_EVENTS);
    config->epoll_timeout_ms = get_env_int("RKLLM_EPOLL_TIMEOUT_MS", DEFAULT_EPOLL_TIMEOUT_MS);
    config->buffer_size = get_env_int("RKLLM_BUFFER_SIZE", DEFAULT_BUFFER_SIZE);
    config->log_level = get_env_int("RKLLM_LOG_LEVEL", DEFAULT_LOG_LEVEL);
    config->connection_buffer_size = get_env_int("RKLLM_CONNECTION_BUFFER_SIZE", DEFAULT_CONNECTION_BUFFER_SIZE);
    config->error_buffer_size = get_env_int("RKLLM_ERROR_BUFFER_SIZE", DEFAULT_ERROR_BUFFER_SIZE);
    config->small_error_buffer_size = get_env_int("RKLLM_SMALL_ERROR_BUFFER_SIZE", DEFAULT_SMALL_ERROR_BUFFER_SIZE);
    config->timestamp_buffer_size = get_env_int("RKLLM_TIMESTAMP_BUFFER_SIZE", DEFAULT_TIMESTAMP_BUFFER_SIZE);
    config->max_path_length = get_env_int("RKLLM_MAX_PATH_LENGTH", DEFAULT_MAX_PATH_LENGTH);
    config->method_name_length = get_env_int("RKLLM_METHOD_NAME_LENGTH", DEFAULT_METHOD_NAME_LENGTH);
    config->init_timeout = get_env_int("RKLLM_INIT_TIMEOUT", DEFAULT_INIT_TIMEOUT);
    config->async_timeout = get_env_int("RKLLM_ASYNC_TIMEOUT", DEFAULT_ASYNC_TIMEOUT);
    
    // Validate socket_path allocation
    if (!config->socket_path) {
        free(config);
        return NULL;
    }
    
    return config;
}

void free_server_config(ServerConfig* config) {
    if (!config) {
        return;
    }
    
    if (config->socket_path) {
        free(config->socket_path);
    }
    
    free(config);
}