#pragma once

#include "settings.h"

/**
 * Global settings access - singleton pattern for settings
 */

/**
 * Initialize global settings instance
 * @param settings Settings to use globally
 * @return 0 on success, -1 on error
 */
int settings_global_init(const mcp_settings_t* settings);

/**
 * Get the global settings instance
 * @return Pointer to global settings, or NULL if not initialized
 */
const mcp_settings_t* settings_global_get(void);

/**
 * Shutdown global settings
 */
void settings_global_shutdown(void);

// Convenience macros for accessing settings
#define SETTINGS() settings_global_get()
#define SETTING_SERVER(field) (SETTINGS() ? SETTINGS()->server.field : 0)
#define SETTING_TRANSPORT(transport, field) (SETTINGS() ? SETTINGS()->transports.transport.field : 0)
#define SETTING_BUFFER(field) (SETTINGS() ? SETTINGS()->buffers.field : 0)
#define SETTING_LIMIT(field) (SETTINGS() ? SETTINGS()->limits.field : 0)
#define SETTING_RKLLM(field) (SETTINGS() ? SETTINGS()->rkllm.field : 0)