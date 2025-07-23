#include "find_connection.h"
#include <stddef.h>

Connection* find_connection(ConnectionManager* manager, int fd) {
    if (!manager) {
        return NULL;
    }
    
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i] && manager->connections[i]->fd == fd) {
            return manager->connections[i];
        }
    }
    
    return NULL;
}