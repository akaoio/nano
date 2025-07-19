#include "handle_pool.h"
#include <time.h>
#include <string.h>

int handle_pool_cleanup(handle_pool_t* pool) {
    if (!pool) return -1;
    
    time_t now = time(nullptr);
    int cleaned = 0;
    
    for (int i = 0; i < MAX_HANDLES; i++) {
        handle_slot_t* slot = &pool->slots[i];
        if (slot->active) {
            // Clean up handles unused for more than 5 minutes
            if (now - slot->last_used > 300) {
                rkllm_destroy(slot->handle);
                pool->total_memory -= slot->memory_usage;
                memset(slot, 0, sizeof(handle_slot_t));
                cleaned++;
            }
        }
    }
    
    return cleaned;
}
