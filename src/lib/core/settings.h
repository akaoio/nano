#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <json-c/json.h>

// Settings structure that mirrors all configurable options
typedef struct {
    // Server settings
    struct {
        char* name;
        char* version;
        bool enable_logging;
        char* log_file;
        char* pid_file;
        bool force_kill_existing;
    } server;
    
    // Transport settings
    struct {
        bool enable_stdio;
        bool enable_tcp;
        bool enable_udp; 
        bool enable_http;
        bool enable_websocket;
        
        struct {
            bool line_buffered;
            bool log_to_stderr;
        } stdio;
        
        struct {
            char* host;
            uint16_t port;
            int timeout_ms;
            int max_retries;
        } tcp;
        
        struct {
            char* host;
            uint16_t port;
            int timeout_ms;
            int max_retries;
        } udp;
        
        struct {
            char* host;
            uint16_t port;
            char* path;
            int timeout_ms;
            bool keep_alive;
            size_t max_header_size;
            size_t max_body_size;
        } http;
        
        struct {
            char* host;
            uint16_t port;
            char* path;
            size_t max_frame_length;
            int ping_interval_ms;
        } websocket;
        
        // Common transport settings
        struct {
            size_t buffer_size;
            int default_timeout_ms;
            int max_retries;
        } common;
    } transports;
    
    // RKLLM default parameters
    struct {
        char* default_model_path;
        int32_t max_context_len;
        int32_t max_new_tokens;
        int32_t top_k;
        int32_t n_keep;
        float top_p;
        float temperature;
        float repeat_penalty;
        float frequency_penalty;
        float presence_penalty;
        int32_t mirostat;
        float mirostat_tau;
        float mirostat_eta;
        bool skip_special_token;
        bool is_async;
        
        // Extended parameters
        struct {
            int32_t base_domain_id;
            int8_t embed_flash;
            int8_t enabled_cpus_num;
            uint32_t enabled_cpus_mask;
            uint8_t n_batch;
            int8_t use_cross_attn;
        } extend;
    } rkllm;
    
    // Buffer and memory settings
    struct {
        size_t request_buffer_size;
        size_t response_buffer_size;
        size_t max_json_size;
        size_t proxy_response_buffer_size;
        size_t proxy_arg_buffer_size;
        size_t transport_buffer_size;
        size_t http_request_buffer_size;
        size_t http_response_buffer_size;
        size_t websocket_message_buffer_size;
        size_t log_message_buffer_size;
    } buffers;
    
    // Limits and constraints
    struct {
        size_t max_request_size;
        size_t max_response_size;
        size_t max_settings_file_size;
        int max_concurrent_connections;
        int max_pending_requests;
    } limits;
    
} mcp_settings_t;

/**
 * Load settings from JSON file
 * @param filepath Path to settings.json file
 * @param settings Output settings structure
 * @return 0 on success, -1 on error
 */
int settings_load_from_file(const char* filepath, mcp_settings_t* settings);

/**
 * Load settings from JSON string
 * @param json_str JSON string containing settings
 * @param settings Output settings structure  
 * @return 0 on success, -1 on error
 */
int settings_load_from_string(const char* json_str, mcp_settings_t* settings);

/**
 * Apply command line overrides to settings
 * @param settings Settings to modify
 * @param argc Argument count
 * @param argv Argument values
 * @return 0 on success, -1 on error
 */
int settings_apply_overrides(mcp_settings_t* settings, int argc, char* argv[]);

/**
 * Free allocated settings memory
 * @param settings Settings to free
 */
void settings_free(mcp_settings_t* settings);

/**
 * Get default settings
 * @param settings Output settings structure
 */
void settings_get_defaults(mcp_settings_t* settings);

/**
 * Generate a complete settings.json file with all available options and comments
 * @param filepath Path to generate settings file
 * @return 0 on success, -1 on error
 */
int settings_generate_complete_file(const char* filepath);

/**
 * Save settings to JSON file
 * @param filepath Path to save settings
 * @param settings Settings to save
 * @return 0 on success, -1 on error
 */
int settings_save_to_file(const char* filepath, const mcp_settings_t* settings);

/**
 * Validate settings
 * @param settings Settings to validate
 * @return 0 if valid, -1 if invalid
 */
int settings_validate(const mcp_settings_t* settings);