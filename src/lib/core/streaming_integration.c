#include "streaming_integration.h"
#include "rkllm_proxy.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global integration manager instance
static streaming_integration_manager_t g_integration_manager = {0};

streaming_integration_result_t streaming_integration_init(void) {
    if (g_integration_manager.initialized) {
        return STREAMING_INTEGRATION_OK;
    }
    
    memset(&g_integration_manager, 0, sizeof(g_integration_manager));
    
    // Initialize default configurations for different transport types
    enhanced_transport_get_optimal_config("websocket", &g_integration_manager.default_websocket_config);
    enhanced_transport_get_optimal_config("http", &g_integration_manager.default_http_config);
    enhanced_transport_get_optimal_config("tcp", &g_integration_manager.default_tcp_config);
    
    g_integration_manager.initialized = true;
    
    printf("✅ Streaming integration system initialized\n");
    return STREAMING_INTEGRATION_OK;
}

void streaming_integration_shutdown(void) {
    if (!g_integration_manager.initialized) {
        return;
    }
    
    // Shutdown all enhanced transport managers
    for (int i = 0; i < g_integration_manager.active_managers_count; i++) {
        if (g_integration_manager.enhanced_managers[i]) {
            enhanced_transport_manager_shutdown(g_integration_manager.enhanced_managers[i]);
            free(g_integration_manager.enhanced_managers[i]);
            g_integration_manager.enhanced_managers[i] = NULL;
        }
    }
    
    g_integration_manager.active_managers_count = 0;
    g_integration_manager.initialized = false;
    
    printf("🧹 Streaming integration system shutdown completed\n");
}

enhanced_transport_manager_t* streaming_integration_create_enhanced_transport(
    transport_manager_t* base_manager, const char* transport_name) {
    
    if (!g_integration_manager.initialized) {
        streaming_integration_init();
    }
    
    if (!base_manager || !transport_name) {
        return NULL;
    }
    
    if (g_integration_manager.active_managers_count >= 8) {
        printf("❌ Maximum number of enhanced transport managers reached\n");
        return NULL;
    }
    
    // Get optimal configuration for transport type
    transport_stream_config_t config;
    if (enhanced_transport_get_optimal_config(transport_name, &config) != 0) {
        printf("❌ Failed to get optimal config for transport: %s\n", transport_name);
        return NULL;
    }
    
    // Create enhanced transport manager
    enhanced_transport_manager_t* enhanced_manager = malloc(sizeof(enhanced_transport_manager_t));
    if (!enhanced_manager) {
        return NULL;
    }
    
    if (enhanced_transport_manager_init(enhanced_manager, base_manager, &config) != 0) {
        free(enhanced_manager);
        return NULL;
    }
    
    // Add to global manager list
    g_integration_manager.enhanced_managers[g_integration_manager.active_managers_count] = enhanced_manager;
    g_integration_manager.active_managers_count++;
    
    printf("🔧 Created enhanced transport manager for %s (total: %d/8)\n", 
           transport_name, g_integration_manager.active_managers_count);
    
    return enhanced_manager;
}

rkllm_stream_context_t* streaming_integration_start_stream(
    uint32_t request_id, LLMHandle handle, enhanced_transport_manager_t* transport_manager) {
    
    if (!g_integration_manager.initialized || !handle || !transport_manager) {
        return NULL;
    }
    
    // Create streaming session
    rkllm_stream_context_t* stream_context = rkllm_stream_create_session(
        request_id, handle, enhanced_transport_streaming_callback, transport_manager);
    
    if (!stream_context) {
        return NULL;
    }
    
    // Attach to transport manager
    if (enhanced_transport_manager_attach_stream(transport_manager, stream_context) != 0) {
        rkllm_stream_destroy_session(stream_context->session_id);
        return NULL;
    }
    
    // Start streaming
    if (rkllm_stream_start(stream_context) != 0) {
        enhanced_transport_manager_detach_stream(transport_manager);
        rkllm_stream_destroy_session(stream_context->session_id);
        return NULL;
    }
    
    g_integration_manager.total_streams_created++;
    g_integration_manager.active_streams_count++;
    
    printf("🎬 Started integrated streaming session: %s via %s transport\n",
           stream_context->session_id, transport_manager->base_manager->transport->name);
    
    return stream_context;
}

streaming_integration_result_t streaming_integration_stop_stream(const char* session_id) {
    if (!g_integration_manager.initialized || !session_id) {
        return STREAMING_INTEGRATION_INVALID_PARAMS;
    }
    
    // Find and stop the stream
    rkllm_stream_context_t* context = rkllm_stream_find_session(session_id);
    if (!context) {
        return STREAMING_INTEGRATION_STREAM_ERROR;
    }
    
    // Abort streaming
    rkllm_stream_abort(context);
    
    // Find associated transport manager and detach
    for (int i = 0; i < g_integration_manager.active_managers_count; i++) {
        enhanced_transport_manager_t* manager = g_integration_manager.enhanced_managers[i];
        if (manager && manager->active_stream_context == context) {
            enhanced_transport_manager_detach_stream(manager);
            break;
        }
    }
    
    // Destroy session
    rkllm_stream_destroy_session(session_id);
    
    if (g_integration_manager.active_streams_count > 0) {
        g_integration_manager.active_streams_count--;
    }
    
    printf("🛑 Stopped integrated streaming session: %s\n", session_id);
    
    return STREAMING_INTEGRATION_OK;
}

streaming_integration_result_t streaming_integration_get_status(
    const char* session_id, char** status_json) {
    
    if (!g_integration_manager.initialized || !session_id || !status_json) {
        return STREAMING_INTEGRATION_INVALID_PARAMS;
    }
    
    rkllm_stream_context_t* context = rkllm_stream_find_session(session_id);
    if (!context) {
        return STREAMING_INTEGRATION_STREAM_ERROR;
    }
    
    // Get stream statistics
    char* stream_stats = NULL;
    if (rkllm_stream_get_stats(context, &stream_stats) != 0) {
        return STREAMING_INTEGRATION_STREAM_ERROR;
    }
    
    // Get transport statistics
    char* transport_stats = NULL;
    for (int i = 0; i < g_integration_manager.active_managers_count; i++) {
        enhanced_transport_manager_t* manager = g_integration_manager.enhanced_managers[i];
        if (manager && manager->active_stream_context == context) {
            if (enhanced_transport_get_streaming_stats(manager, &transport_stats) != 0) {
                transport_stats = strdup("{}");
            }
            break;
        }
    }
    
    if (!transport_stats) {
        transport_stats = strdup("{}");
    }
    
    // Combine statistics
    json_object* combined_status = json_object_new_object();
    json_object_object_add(combined_status, "integration_info", json_object_new_object());
    
    // Add stream stats
    json_object* stream_obj = json_tokener_parse(stream_stats);
    if (stream_obj) {
        json_object_object_add(combined_status, "stream_stats", stream_obj);
    }
    
    // Add transport stats
    json_object* transport_obj = json_tokener_parse(transport_stats);
    if (transport_obj) {
        json_object_object_add(combined_status, "transport_stats", transport_obj);
    }
    
    // Add integration stats
    json_object* integration_obj;
    json_object_object_get_ex(combined_status, "integration_info", &integration_obj);
    json_object_object_add(integration_obj, "total_streams_created", 
                          json_object_new_int(g_integration_manager.total_streams_created));
    json_object_object_add(integration_obj, "active_streams_count", 
                          json_object_new_int(g_integration_manager.active_streams_count));
    json_object_object_add(integration_obj, "active_transports", 
                          json_object_new_int(g_integration_manager.active_managers_count));
    
    const char* json_str = json_object_to_json_string(combined_status);
    *status_json = strdup(json_str);
    json_object_put(combined_status);
    
    free(stream_stats);
    free(transport_stats);
    
    return STREAMING_INTEGRATION_OK;
}

streaming_integration_result_t streaming_integration_list_active_streams(char** sessions_json) {
    if (!g_integration_manager.initialized || !sessions_json) {
        return STREAMING_INTEGRATION_INVALID_PARAMS;
    }
    
    json_object* sessions_array = json_object_new_array();
    
    // This would require extending the streaming context manager to iterate sessions
    // For now, return basic info
    json_object* info = json_object_new_object();
    json_object_object_add(info, "active_streams_count", 
                          json_object_new_int(g_integration_manager.active_streams_count));
    json_object_object_add(info, "total_streams_created", 
                          json_object_new_int(g_integration_manager.total_streams_created));
    json_object_object_add(info, "active_transports", 
                          json_object_new_int(g_integration_manager.active_managers_count));
    
    json_object_array_add(sessions_array, info);
    
    const char* json_str = json_object_to_json_string(sessions_array);
    *sessions_json = strdup(json_str);
    json_object_put(sessions_array);
    
    return STREAMING_INTEGRATION_OK;
}

streaming_integration_result_t streaming_integration_handle_request(
    const char* method, const char* params_json, uint32_t request_id,
    enhanced_transport_manager_t* transport_manager, char** result_json) {
    
    if (!method || !result_json) {
        return STREAMING_INTEGRATION_INVALID_PARAMS;
    }
    
    if (!g_integration_manager.initialized) {
        streaming_integration_init();
    }
    
    // Handle streaming-specific methods
    if (strcmp(method, "streaming_create_session") == 0) {
        LLMHandle handle = rkllm_proxy_get_handle();
        if (!handle) {
            *result_json = strdup("{\"error\": \"RKLLM not initialized\"}");
            return STREAMING_INTEGRATION_ERROR;
        }
        
        rkllm_stream_context_t* context = streaming_integration_start_stream(
            request_id, handle, transport_manager);
        
        if (context) {
            json_object* result = json_object_new_object();
            json_object_object_add(result, "session_id", json_object_new_string(context->session_id));
            json_object_object_add(result, "status", json_object_new_string("created"));
            
            const char* json_str = json_object_to_json_string(result);
            *result_json = strdup(json_str);
            json_object_put(result);
            
            return STREAMING_INTEGRATION_OK;
        } else {
            *result_json = strdup("{\"error\": \"Failed to create streaming session\"}");
            return STREAMING_INTEGRATION_ERROR;
        }
        
    } else if (strcmp(method, "streaming_list_sessions") == 0) {
        return streaming_integration_list_active_streams(result_json);
        
    } else if (strcmp(method, "streaming_get_status") == 0) {
        // Extract session_id from params
        json_object* params = json_tokener_parse(params_json);
        json_object* session_id_obj;
        
        if (params && json_object_object_get_ex(params, "session_id", &session_id_obj)) {
            const char* session_id = json_object_get_string(session_id_obj);
            streaming_integration_result_t result = streaming_integration_get_status(session_id, result_json);
            json_object_put(params);
            return result;
        }
        
        if (params) json_object_put(params);
        *result_json = strdup("{\"error\": \"Missing session_id parameter\"}");
        return STREAMING_INTEGRATION_INVALID_PARAMS;
        
    } else {
        // Not a streaming method, let regular proxy handle it
        return STREAMING_INTEGRATION_ERROR;
    }
}