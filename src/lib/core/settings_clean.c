#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char* safe_strdup(const char* str) {
    if (!str) return NULL;
    return strdup(str);
}

static void set_string_field(char** field, json_object* obj, const char* key, const char* default_val) {
    json_object* val;
    if (json_object_object_get_ex(obj, key, &val) && json_object_get_type(val) == json_type_string) {
        free(*field);
        *field = safe_strdup(json_object_get_string(val));
    } else if (default_val) {
        free(*field);
        *field = safe_strdup(default_val);
    }
}

static int get_int_field(json_object* obj, const char* key, int default_val) {
    json_object* val;
    if (json_object_object_get_ex(obj, key, &val) && json_object_get_type(val) == json_type_int) {
        return json_object_get_int(val);
    }
    return default_val;
}

static bool get_bool_field(json_object* obj, const char* key, bool default_val) {
    json_object* val;
    if (json_object_object_get_ex(obj, key, &val) && json_object_get_type(val) == json_type_boolean) {
        return json_object_get_boolean(val);
    }
    return default_val;
}

static double get_double_field(json_object* obj, const char* key, double default_val) {
    json_object* val;
    if (json_object_object_get_ex(obj, key, &val)) {
        if (json_object_get_type(val) == json_type_double) {
            return json_object_get_double(val);
        } else if (json_object_get_type(val) == json_type_int) {
            return (double)json_object_get_int(val);
        }
    }
    return default_val;
}

void settings_get_defaults(mcp_settings_t* settings) {
    if (!settings) return;
    
    memset(settings, 0, sizeof(mcp_settings_t));
    
    // Server defaults
    settings->server.name = safe_strdup("MCP-Server");
    settings->server.version = safe_strdup("1.0.0");
    settings->server.enable_logging = true;
    settings->server.log_file = NULL; // stderr
    settings->server.pid_file = safe_strdup("/tmp/mcp_server.pid");
    settings->server.force_kill_existing = false;
    
    // Transport defaults
    settings->transports.enable_stdio = true;
    settings->transports.enable_tcp = true;
    settings->transports.enable_udp = true;
    settings->transports.enable_http = true;
    settings->transports.enable_websocket = true;
    
    settings->transports.stdio.line_buffered = true;
    settings->transports.stdio.log_to_stderr = false;
    
    settings->transports.tcp.host = safe_strdup("0.0.0.0");
    settings->transports.tcp.port = 8080;
    settings->transports.tcp.timeout_ms = 5000;
    settings->transports.tcp.max_retries = 3;
    
    settings->transports.udp.host = safe_strdup("0.0.0.0");
    settings->transports.udp.port = 8081;
    settings->transports.udp.timeout_ms = 5000;
    
    settings->transports.http.host = safe_strdup("0.0.0.0");
    settings->transports.http.port = 8082;
    settings->transports.http.path = safe_strdup("/");
    settings->transports.http.timeout_ms = 30000;
    settings->transports.http.keep_alive = true;
    settings->transports.http.max_header_size = 8192;
    settings->transports.http.max_body_size = 1024 * 1024;  // 1MB
    
    settings->transports.websocket.host = safe_strdup("0.0.0.0");
    settings->transports.websocket.port = 8083;
    settings->transports.websocket.path = safe_strdup("/");
    settings->transports.websocket.max_frame_length = 16 * 1024 * 1024;  // 16MB
    settings->transports.websocket.ping_interval_ms = 30000;
    
    settings->transports.common.buffer_size = 8192;
    settings->transports.common.default_timeout_ms = 5000;
    settings->transports.common.max_retries = 3;
    
    // RKLLM defaults
    settings->rkllm.default_model_path = safe_strdup("models/qwen3/model.rkllm");
    settings->rkllm.max_context_len = 512;
    settings->rkllm.max_new_tokens = 256;
    settings->rkllm.top_k = 40;
    settings->rkllm.n_keep = 0;
    settings->rkllm.top_p = 0.9f;
    settings->rkllm.temperature = 0.8f;
    settings->rkllm.repeat_penalty = 1.1f;
    settings->rkllm.frequency_penalty = 0.0f;
    settings->rkllm.presence_penalty = 0.0f;
    settings->rkllm.mirostat = 0;
    settings->rkllm.mirostat_tau = 5.0f;
    settings->rkllm.mirostat_eta = 0.1f;
    settings->rkllm.skip_special_token = false;
    settings->rkllm.is_async = false;
    
    // RKLLM extend defaults
    settings->rkllm.extend.base_domain_id = 0;
    settings->rkllm.extend.embed_flash = 0;
    settings->rkllm.extend.enabled_cpus_num = 4;
    settings->rkllm.extend.enabled_cpus_mask = 0xF0; // CPU 4-7
    settings->rkllm.extend.n_batch = 1;
    settings->rkllm.extend.use_cross_attn = 0;
    
    // Buffer defaults
    settings->buffers.request_buffer_size = 8192;
    settings->buffers.response_buffer_size = 16384;
    settings->buffers.max_json_size = 65536;
    settings->buffers.proxy_response_buffer_size = 8192;
    settings->buffers.proxy_arg_buffer_size = 1024;
    settings->buffers.transport_buffer_size = 8192;
    settings->buffers.http_request_buffer_size = 8192;
    settings->buffers.http_response_buffer_size = 8192;
    settings->buffers.websocket_message_buffer_size = 8192;
    settings->buffers.log_message_buffer_size = 128;
    
    // Limits defaults
    settings->limits.max_request_size = 4096;
    settings->limits.max_response_size = 8192;
    settings->limits.max_settings_file_size = 1024 * 1024;  // 1MB
    settings->limits.max_concurrent_connections = 100;
    settings->limits.max_pending_requests = 1000;
}

static int load_server_settings(json_object* root, mcp_settings_t* settings) {
    json_object* server;
    if (!json_object_object_get_ex(root, "server", &server)) {
        return 0; // No server section, use defaults
    }
    
    set_string_field(&settings->server.name, server, "name", "MCP-Server");
    set_string_field(&settings->server.version, server, "version", "1.0.0");
    settings->server.enable_logging = get_bool_field(server, "enable_logging", true);
    set_string_field(&settings->server.log_file, server, "log_file", NULL);
    set_string_field(&settings->server.pid_file, server, "pid_file", "/tmp/mcp_server.pid");
    settings->server.force_kill_existing = get_bool_field(server, "force_kill_existing", false);
    
    return 0;
}

static int load_transport_settings(json_object* root, mcp_settings_t* settings) {
    json_object* transports;
    if (!json_object_object_get_ex(root, "transports", &transports)) {
        return 0; // No transports section, use defaults
    }
    
    // General transport settings
    settings->transports.enable_stdio = get_bool_field(transports, "enable_stdio", true);
    settings->transports.enable_tcp = get_bool_field(transports, "enable_tcp", true);
    settings->transports.enable_udp = get_bool_field(transports, "enable_udp", true);
    settings->transports.enable_http = get_bool_field(transports, "enable_http", true);
    settings->transports.enable_websocket = get_bool_field(transports, "enable_websocket", true);
    
    // STDIO settings
    json_object* stdio;
    if (json_object_object_get_ex(transports, "stdio", &stdio)) {
        settings->transports.stdio.line_buffered = get_bool_field(stdio, "line_buffered", true);
        settings->transports.stdio.log_to_stderr = get_bool_field(stdio, "log_to_stderr", false);
    }
    
    // TCP settings
    json_object* tcp;
    if (json_object_object_get_ex(transports, "tcp", &tcp)) {
        set_string_field(&settings->transports.tcp.host, tcp, "host", "0.0.0.0");
        settings->transports.tcp.port = get_int_field(tcp, "port", 8080);
        settings->transports.tcp.timeout_ms = get_int_field(tcp, "timeout_ms", 5000);
        settings->transports.tcp.max_retries = get_int_field(tcp, "max_retries", 3);
    }
    
    // UDP settings
    json_object* udp;
    if (json_object_object_get_ex(transports, "udp", &udp)) {
        set_string_field(&settings->transports.udp.host, udp, "host", "0.0.0.0");
        settings->transports.udp.port = get_int_field(udp, "port", 8081);
        settings->transports.udp.timeout_ms = get_int_field(udp, "timeout_ms", 5000);
    }
    
    // HTTP settings
    json_object* http;
    if (json_object_object_get_ex(transports, "http", &http)) {
        set_string_field(&settings->transports.http.host, http, "host", "0.0.0.0");
        settings->transports.http.port = get_int_field(http, "port", 8082);
        set_string_field(&settings->transports.http.path, http, "path", "/");
        settings->transports.http.timeout_ms = get_int_field(http, "timeout_ms", 30000);
        settings->transports.http.keep_alive = get_bool_field(http, "keep_alive", true);
        settings->transports.http.max_header_size = get_int_field(http, "max_header_size", 8192);
        settings->transports.http.max_body_size = get_int_field(http, "max_body_size", 1024 * 1024);
    }
    
    // WebSocket settings
    json_object* ws;
    if (json_object_object_get_ex(transports, "websocket", &ws)) {
        set_string_field(&settings->transports.websocket.host, ws, "host", "0.0.0.0");
        settings->transports.websocket.port = get_int_field(ws, "port", 8083);
        set_string_field(&settings->transports.websocket.path, ws, "path", "/");
        settings->transports.websocket.max_frame_length = get_int_field(ws, "max_frame_length", 16 * 1024 * 1024);
        settings->transports.websocket.ping_interval_ms = get_int_field(ws, "ping_interval_ms", 30000);
    }
    
    // Common transport settings
    json_object* common;
    if (json_object_object_get_ex(transports, "common", &common)) {
        settings->transports.common.buffer_size = get_int_field(common, "buffer_size", 8192);
        settings->transports.common.default_timeout_ms = get_int_field(common, "default_timeout_ms", 5000);
        settings->transports.common.max_retries = get_int_field(common, "max_retries", 3);
    }
    
    return 0;
}

static int load_rkllm_settings(json_object* root, mcp_settings_t* settings) {
    json_object* rkllm;
    if (!json_object_object_get_ex(root, "rkllm", &rkllm)) {
        return 0; // No RKLLM section, use defaults
    }
    
    set_string_field(&settings->rkllm.default_model_path, rkllm, "default_model_path", "models/qwen3/model.rkllm");
    settings->rkllm.max_context_len = get_int_field(rkllm, "max_context_len", 512);
    settings->rkllm.max_new_tokens = get_int_field(rkllm, "max_new_tokens", 256);
    settings->rkllm.top_k = get_int_field(rkllm, "top_k", 40);
    settings->rkllm.n_keep = get_int_field(rkllm, "n_keep", 0);
    settings->rkllm.top_p = get_double_field(rkllm, "top_p", 0.9);
    settings->rkllm.temperature = get_double_field(rkllm, "temperature", 0.8);
    settings->rkllm.repeat_penalty = get_double_field(rkllm, "repeat_penalty", 1.1);
    settings->rkllm.frequency_penalty = get_double_field(rkllm, "frequency_penalty", 0.0);
    settings->rkllm.presence_penalty = get_double_field(rkllm, "presence_penalty", 0.0);
    settings->rkllm.mirostat = get_int_field(rkllm, "mirostat", 0);
    settings->rkllm.mirostat_tau = get_double_field(rkllm, "mirostat_tau", 5.0);
    settings->rkllm.mirostat_eta = get_double_field(rkllm, "mirostat_eta", 0.1);
    settings->rkllm.skip_special_token = get_bool_field(rkllm, "skip_special_token", false);
    settings->rkllm.is_async = get_bool_field(rkllm, "is_async", false);
    
    // Extended parameters
    json_object* extend;
    if (json_object_object_get_ex(rkllm, "extend", &extend)) {
        settings->rkllm.extend.base_domain_id = get_int_field(extend, "base_domain_id", 0);
        settings->rkllm.extend.embed_flash = get_int_field(extend, "embed_flash", 0);
        settings->rkllm.extend.enabled_cpus_num = get_int_field(extend, "enabled_cpus_num", 4);
        settings->rkllm.extend.enabled_cpus_mask = get_int_field(extend, "enabled_cpus_mask", 0xF0);
        settings->rkllm.extend.n_batch = get_int_field(extend, "n_batch", 1);
        settings->rkllm.extend.use_cross_attn = get_int_field(extend, "use_cross_attn", 0);
    }
    
    return 0;
}

static int load_buffer_settings(json_object* root, mcp_settings_t* settings) {
    json_object* buffers;
    if (!json_object_object_get_ex(root, "buffers", &buffers)) {
        return 0; // No buffer section, use defaults
    }
    
    settings->buffers.request_buffer_size = get_int_field(buffers, "request_buffer_size", 8192);
    settings->buffers.response_buffer_size = get_int_field(buffers, "response_buffer_size", 16384);
    settings->buffers.max_json_size = get_int_field(buffers, "max_json_size", 65536);
    settings->buffers.proxy_response_buffer_size = get_int_field(buffers, "proxy_response_buffer_size", 8192);
    settings->buffers.proxy_arg_buffer_size = get_int_field(buffers, "proxy_arg_buffer_size", 1024);
    settings->buffers.transport_buffer_size = get_int_field(buffers, "transport_buffer_size", 8192);
    settings->buffers.http_request_buffer_size = get_int_field(buffers, "http_request_buffer_size", 8192);
    settings->buffers.http_response_buffer_size = get_int_field(buffers, "http_response_buffer_size", 8192);
    settings->buffers.websocket_message_buffer_size = get_int_field(buffers, "websocket_message_buffer_size", 8192);
    settings->buffers.log_message_buffer_size = get_int_field(buffers, "log_message_buffer_size", 128);
    
    return 0;
}

static int load_limits_settings(json_object* root, mcp_settings_t* settings) {
    json_object* limits;
    if (!json_object_object_get_ex(root, "limits", &limits)) {
        return 0; // No limits section, use defaults
    }
    
    settings->limits.max_request_size = get_int_field(limits, "max_request_size", 4096);
    settings->limits.max_response_size = get_int_field(limits, "max_response_size", 8192);
    settings->limits.max_settings_file_size = get_int_field(limits, "max_settings_file_size", 1024 * 1024);
    settings->limits.max_concurrent_connections = get_int_field(limits, "max_concurrent_connections", 100);
    settings->limits.max_pending_requests = get_int_field(limits, "max_pending_requests", 1000);
    
    return 0;
}

int settings_load_from_string(const char* json_str, mcp_settings_t* settings) {
    if (!json_str || !settings) {
        return -1;
    }
    
    json_object* root = json_tokener_parse(json_str);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON settings\n");
        return -1;
    }
    
    // Start with defaults
    settings_get_defaults(settings);
    
    // Load each section
    int ret = 0;
    ret |= load_server_settings(root, settings);
    ret |= load_transport_settings(root, settings);
    ret |= load_rkllm_settings(root, settings);
    ret |= load_buffer_settings(root, settings);
    ret |= load_limits_settings(root, settings);
    
    json_object_put(root);
    return ret;
}

int settings_load_from_file(const char* filepath, mcp_settings_t* settings) {
    if (!filepath || !settings) {
        return -1;
    }
    
    // Check if file exists
    if (access(filepath, R_OK) != 0) {
        printf("⚠️  Settings file %s not found, generating complete configuration...\n", filepath);
        
        // Generate complete settings file with all available options
        if (settings_generate_complete_file(filepath) == 0) {
            printf("ℹ️  Complete settings.json generated. You can now customize it as needed.\n");
            printf("ℹ️  All available configuration options are included with their default values.\n");
        } else {
            printf("❌ Failed to generate settings file, using defaults\n");
        }
        
        settings_get_defaults(settings);
        return 0; // Continue with defaults for this run
    }
    
    FILE* file = fopen(filepath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open settings file: %s\n", filepath);
        settings_get_defaults(settings);
        return 0; // Use defaults on error
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size <= 0 || size > 1024 * 1024) { // Max 1MB settings file
        fprintf(stderr, "Invalid settings file size: %ld\n", size);
        fclose(file);
        settings_get_defaults(settings);
        return 0;
    }
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for settings\n");
        fclose(file);
        return -1;
    }
    
    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';
    fclose(file);
    
    int ret = settings_load_from_string(buffer, settings);
    free(buffer);
    
    return ret;
}

int settings_apply_overrides(mcp_settings_t* settings, int argc, char* argv[]) {
    if (!settings || !argv) return -1;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--force") == 0) {
            settings->server.force_kill_existing = true;
        } else if (strcmp(argv[i], "--disable-stdio") == 0) {
            settings->transports.enable_stdio = false;
        } else if (strcmp(argv[i], "--disable-tcp") == 0) {
            settings->transports.enable_tcp = false;
        } else if (strcmp(argv[i], "--disable-udp") == 0) {
            settings->transports.enable_udp = false;
        } else if (strcmp(argv[i], "--disable-http") == 0) {
            settings->transports.enable_http = false;
        } else if (strcmp(argv[i], "--disable-ws") == 0) {
            settings->transports.enable_websocket = false;
        } else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tcp") == 0) && i + 1 < argc) {
            settings->transports.tcp.port = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--udp") == 0) && i + 1 < argc) {
            settings->transports.udp.port = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--http") == 0) && i + 1 < argc) {
            settings->transports.http.port = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--ws") == 0) && i + 1 < argc) {
            settings->transports.websocket.port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            free(settings->server.log_file);
            settings->server.log_file = safe_strdup(argv[++i]);
        }
    }
    
    return 0;
}

void settings_free(mcp_settings_t* settings) {
    if (!settings) return;
    
    // Free server strings
    free(settings->server.name);
    free(settings->server.version);
    free(settings->server.log_file);
    free(settings->server.pid_file);
    
    // Free transport strings
    free(settings->transports.tcp.host);
    free(settings->transports.udp.host);
    free(settings->transports.http.host);
    free(settings->transports.http.path);
    free(settings->transports.websocket.host);
    free(settings->transports.websocket.path);
    
    // Free RKLLM strings
    free(settings->rkllm.default_model_path);
    
    // Zero out the structure
    memset(settings, 0, sizeof(mcp_settings_t));
}

int settings_validate(const mcp_settings_t* settings) {
    if (!settings) return -1;
    
    // Validate ports are in valid range
    if (settings->transports.tcp.port < 1 || settings->transports.tcp.port > 65535) {
        fprintf(stderr, "Invalid TCP port: %d\n", settings->transports.tcp.port);
        return -1;
    }
    if (settings->transports.udp.port < 1 || settings->transports.udp.port > 65535) {
        fprintf(stderr, "Invalid UDP port: %d\n", settings->transports.udp.port);
        return -1;
    }
    if (settings->transports.http.port < 1 || settings->transports.http.port > 65535) {
        fprintf(stderr, "Invalid HTTP port: %d\n", settings->transports.http.port);
        return -1;
    }
    if (settings->transports.websocket.port < 1 || settings->transports.websocket.port > 65535) {
        fprintf(stderr, "Invalid WebSocket port: %d\n", settings->transports.websocket.port);
        return -1;
    }
    
    // Validate buffer sizes
    if (settings->buffers.request_buffer_size < 1024 || settings->buffers.request_buffer_size > 1024 * 1024) {
        fprintf(stderr, "Invalid request buffer size: %zu\n", settings->buffers.request_buffer_size);
        return -1;
    }
    if (settings->buffers.response_buffer_size < 1024 || settings->buffers.response_buffer_size > 10 * 1024 * 1024) {
        fprintf(stderr, "Invalid response buffer size: %zu\n", settings->buffers.response_buffer_size);
        return -1;
    }
    
    return 0;
}
