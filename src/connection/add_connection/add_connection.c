#include "add_connection.h"
#include <stddef.h>

int add_connection(ConnectionManager* manager, Connection* conn) {
    if (!manager || !conn || manager->count >= manager->max_connections) {
        return -1;
    }
    
    for (int i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i] == NULL) {
            manager->connections[i] = conn;
            manager->count++;
            return 0;
        }
    }
    
    return -1;
}