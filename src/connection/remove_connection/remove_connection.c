#include "remove_connection.h"
#include <stdlib.h>
#include <stddef.h>

int remove_connection(ConnectionManager* manager, int fd) {
    if (!manager) {
        return -1;
    }
    
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i] && manager->connections[i]->fd == fd) {
            free(manager->connections[i]);
            manager->connections[i] = NULL;
            manager->count--;
            return 0;
        }
    }
    
    return -1;
}