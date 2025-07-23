#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include "../../config/get_server_config/get_server_config.h"

/**
 * Sets the global configuration instance
 * Must be called once during initialization
 * @param config Server configuration instance
 */
void set_global_config(ServerConfig* config);

/**
 * Gets the global configuration instance
 * @return Global server configuration, or NULL if not set
 */
ServerConfig* get_global_config(void);

/**
 * Convenience functions for accessing common config values
 */
int get_connection_buffer_size(void);
int get_error_buffer_size(void);
int get_small_error_buffer_size(void);
int get_timestamp_buffer_size(void);
int get_max_path_length(void);
int get_method_name_length(void);
int get_init_timeout(void);
int get_async_timeout(void);

#endif