#include "send_to_connection.h"
#include <sys/socket.h>
#include <unistd.h>

int send_to_connection(Connection* conn, const void* data, size_t len) {
    if (!conn || !data || !conn->is_active) {
        return -1;
    }
    
    return send(conn->fd, data, len, MSG_NOSIGNAL);
}