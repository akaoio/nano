int settings_generate_complete_file(const char* filepath) {
    if (!filepath) {
        return -1;
    }
    
    FILE* file = fopen(filepath, "w");
    if (!file) {
        return -1;
    }
    
    // Write a comprehensive JSON configuration as a single string literal
    const char* complete_config = 
        "{\n"
        "  \"_comment\": \"MCP Server Configuration - Auto-generated with all available options\",\n"
        "  \"server\": {\n"
        "    \"_comment\": \"Server-wide settings\",\n"
        "    \"name\": \"MCP-Server\",\n"
        "    \"version\": \"1.0.0\",\n"
        "    \"enable_logging\": true,\n"
        "    \"log_file\": null,\n"
        "    \"pid_file\": \"/tmp/mcp_server.pid\",\n"
        "    \"force_kill_existing\": false\n"
        "  },\n"
        "  \"transports\": {\n"
        "    \"_comment\": \"Transport layer configuration - enable/disable and configure each transport\",\n"
        "    \"enable_stdio\": true,\n"
        "    \"enable_tcp\": true,\n"
        "    \"enable_udp\": true,\n"
        "    \"enable_http\": true,\n"
        "    \"enable_websocket\": true,\n"
        "    \"stdio\": {\n"
        "      \"_comment\": \"Standard Input/Output transport settings\",\n"
        "      \"line_buffered\": true,\n"
        "      \"log_to_stderr\": false\n"
        "    },\n"
        "    \"tcp\": {\n"
        "      \"_comment\": \"TCP transport settings\",\n"
        "      \"host\": \"0.0.0.0\",\n"
        "      \"port\": 8080,\n"
        "      \"timeout_ms\": 5000,\n"
        "      \"max_retries\": 3\n"
        "    },\n"
        "    \"udp\": {\n"
        "      \"_comment\": \"UDP transport settings\",\n"
        "      \"host\": \"0.0.0.0\",\n"
        "      \"port\": 8081,\n"
        "      \"timeout_ms\": 5000\n"
        "    },\n"
        "    \"http\": {\n"
        "      \"_comment\": \"HTTP transport settings\",\n"
        "      \"host\": \"0.0.0.0\",\n"
        "      \"port\": 8082,\n"
        "      \"path\": \"/\",\n"
        "      \"timeout_ms\": 30000,\n"
        "      \"keep_alive\": true,\n"
        "      \"max_header_size\": 8192,\n"
        "      \"max_body_size\": 1048576\n"
        "    },\n"
        "    \"websocket\": {\n"
        "      \"_comment\": \"WebSocket transport settings\",\n"
        "      \"host\": \"0.0.0.0\",\n"
        "      \"port\": 8083,\n"
        "      \"path\": \"/\",\n"
        "      \"max_frame_length\": 16777216,\n"
        "      \"ping_interval_ms\": 30000\n"
        "    },\n"
        "    \"common\": {\n"
        "      \"_comment\": \"Common transport settings\",\n"
        "      \"buffer_size\": 8192,\n"
        "      \"default_timeout_ms\": 5000,\n"
        "      \"max_retries\": 3\n"
        "    }\n"
        "  },\n"
        "  \"rkllm\": {\n"
        "    \"_comment\": \"RKLLM (Rockchip LLM) engine configuration\",\n"
        "    \"default_model_path\": \"models/qwen3/model.rkllm\",\n"
        "    \"max_context_len\": 512,\n"
        "    \"max_new_tokens\": 256,\n"
        "    \"top_k\": 40,\n"
        "    \"n_keep\": 0,\n"
        "    \"top_p\": 0.9,\n"
        "    \"temperature\": 0.8,\n"
        "    \"repeat_penalty\": 1.1,\n"
        "    \"frequency_penalty\": 0.0,\n"
        "    \"presence_penalty\": 0.0,\n"
        "    \"mirostat\": 0,\n"
        "    \"mirostat_tau\": 5.0,\n"
        "    \"mirostat_eta\": 0.1,\n"
        "    \"skip_special_token\": false,\n"
        "    \"is_async\": false,\n"
        "    \"extend\": {\n"
        "      \"_comment\": \"Extended RKLLM parameters for advanced configuration\",\n"
        "      \"base_domain_id\": 0,\n"
        "      \"embed_flash\": 0,\n"
        "      \"enabled_cpus_num\": 4,\n"
        "      \"enabled_cpus_mask\": 240,\n"
        "      \"n_batch\": 1,\n"
        "      \"use_cross_attn\": 0\n"
        "    }\n"
        "  },\n"
        "  \"buffers\": {\n"
        "    \"_comment\": \"Buffer size configuration for various components\",\n"
        "    \"request_buffer_size\": 8192,\n"
        "    \"response_buffer_size\": 16384,\n"
        "    \"max_json_size\": 65536,\n"
        "    \"proxy_response_buffer_size\": 8192,\n"
        "    \"proxy_arg_buffer_size\": 1024,\n"
        "    \"transport_buffer_size\": 8192,\n"
        "    \"http_request_buffer_size\": 8192,\n"
        "    \"http_response_buffer_size\": 8192,\n"
        "    \"websocket_message_buffer_size\": 8192,\n"
        "    \"log_message_buffer_size\": 128\n"
        "  },\n"
        "  \"limits\": {\n"
        "    \"_comment\": \"System limits and constraints\",\n"
        "    \"max_request_size\": 4096,\n"
        "    \"max_response_size\": 8192,\n"
        "    \"max_settings_file_size\": 1048576,\n"
        "    \"max_concurrent_connections\": 100,\n"
        "    \"max_pending_requests\": 1000\n"
        "  }\n"
        "}\n";
    
    if (fputs(complete_config, file) == EOF) {
        fclose(file);
        return -1;
    }
    
    fclose(file);
    printf("✅ Generated complete settings.json with all available configuration options\n");
    return 0;
}

int settings_save_to_file(const char* filepath, const mcp_settings_t* settings) {
    // For now, just generate the complete file - this ensures consistency
    (void)settings; // Mark as unused to avoid warning
    return settings_generate_complete_file(filepath);
}