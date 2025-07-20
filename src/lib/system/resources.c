#include "resources.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int resource_mgr_init(resource_mgr_t* mgr) {
    if (!mgr) return -1;
    
    // Initialize system info
    if (system_detect(&mgr->system_info) != 0) {
        return -1;
    }
    
    // Initialize model slots
    for (int i = 0; i < MAX_MODELS; i++) {
        mgr->models[i].handle_id = 0;
        mgr->models[i].active = false;
        mgr->models[i].last_used = 0;
    }
    
    mgr->model_count = 0;
    mgr->total_memory_used = 0;
    
    return 0;
}

int resource_mgr_can_load_model(resource_mgr_t* mgr, const char* model_path) {
    if (!mgr || !model_path) return 0;
    
    // Analyze model requirements
    model_info_t model_info;
    if (model_analyze(model_path, &mgr->system_info, &model_info) != 0) {
        return 0;
    }
    
    // Check if we have slot available
    if (mgr->model_count >= MAX_MODELS) {
        return 0;
    }
    
    // Check system resources
    return system_can_load_model(&mgr->system_info, &model_info);
}

int resource_mgr_reserve_model(resource_mgr_t* mgr, uint32_t handle_id, const char* model_path) {
    if (!mgr || !model_path) return -1;
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < MAX_MODELS; i++) {
        if (!mgr->models[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return -1;
    }
    
    // Analyze model
    if (model_analyze(model_path, &mgr->system_info, &mgr->models[slot].model_info) != 0) {
        return -1;
    }
    
    // Reserve resources
    mgr->models[slot].handle_id = handle_id;
    mgr->models[slot].active = true;
    mgr->models[slot].last_used = time(nullptr);
    
    mgr->model_count++;
    mgr->total_memory_used += mgr->models[slot].model_info.memory_required_mb;
    
    
    return 0;
}

int resource_mgr_release_model(resource_mgr_t* mgr, uint32_t handle_id) {
    if (!mgr) return -1;
    
    // Find model slot
    for (int i = 0; i < MAX_MODELS; i++) {
        if (mgr->models[i].active && mgr->models[i].handle_id == handle_id) {
            mgr->total_memory_used -= mgr->models[i].model_info.memory_required_mb;
            mgr->models[i].active = false;
            mgr->models[i].handle_id = 0;
            mgr->model_count--;
            
            return 0;
        }
    }
    
    return -1;
}

uint64_t resource_mgr_get_memory_usage(resource_mgr_t* mgr) {
    if (!mgr) return 0;
    return mgr->total_memory_used;
}

int resource_mgr_cleanup(resource_mgr_t* mgr) {
    if (!mgr) return -1;
    
    // Force system memory cleanup
    system_force_gc();
    system_free_memory();
    
    // Refresh system info
    system_refresh_memory_info(&mgr->system_info);
    
    return 0;
}
