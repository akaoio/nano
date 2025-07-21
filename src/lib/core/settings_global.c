#include "settings_global.h"
#include <stdio.h>
#include <string.h>

static mcp_settings_t* g_global_settings = NULL;
static mcp_settings_t g_settings_copy = {0};

int settings_global_init(const mcp_settings_t* settings) {
    if (!settings) {
        return -1;
    }
    
    // Create a deep copy of the settings
    memcpy(&g_settings_copy, settings, sizeof(mcp_settings_t));
    
    // Deep copy strings (note: this is a simplified approach)
    // In a full implementation, we'd need to duplicate all strings
    g_global_settings = &g_settings_copy;
    
    return 0;
}

const mcp_settings_t* settings_global_get(void) {
    return g_global_settings;
}

void settings_global_shutdown(void) {
    g_global_settings = NULL;
    memset(&g_settings_copy, 0, sizeof(mcp_settings_t));
}