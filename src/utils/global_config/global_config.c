#include "global_config.h"
#include <stddef.h>

static ServerConfig* global_config = NULL;

void set_global_config(ServerConfig* config) {
    global_config = config;
}

ServerConfig* get_global_config(void) {
    return global_config;
}

int get_connection_buffer_size(void) {
    return global_config ? global_config->connection_buffer_size : 8192;
}

int get_error_buffer_size(void) {
    return global_config ? global_config->error_buffer_size : 512;
}

int get_small_error_buffer_size(void) {
    return global_config ? global_config->small_error_buffer_size : 256;
}

int get_timestamp_buffer_size(void) {
    return global_config ? global_config->timestamp_buffer_size : 64;
}

int get_max_path_length(void) {
    return global_config ? global_config->max_path_length : 4096;
}

int get_method_name_length(void) {
    return global_config ? global_config->method_name_length : 128;
}

int get_init_timeout(void) {
    return global_config ? global_config->init_timeout : 30;
}

int get_async_timeout(void) {
    return global_config ? global_config->async_timeout : 60;
}