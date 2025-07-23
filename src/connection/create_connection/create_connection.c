#include "create_connection.h"
#include <stdlib.h>
#include <string.h>

Connection* create_connection(int fd) {
    Connection* conn = malloc(sizeof(Connection));
    if (!conn) {
        return NULL;
    }
    
    conn->fd = fd;
    conn->buffer_len = 0;
    conn->is_active = 1;
    memset(conn->buffer, 0, sizeof(conn->buffer));
    
    return conn;
}