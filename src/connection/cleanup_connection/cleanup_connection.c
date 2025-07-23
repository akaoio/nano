#include "cleanup_connection.h"
#include <unistd.h>
#include <stdlib.h>

void cleanup_connection(Connection* conn) {
    if (!conn) {
        return;
    }
    
    if (conn->fd >= 0) {
        close(conn->fd);
    }
    
    free(conn);
}