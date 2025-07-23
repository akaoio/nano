#include "accept_connection.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <stddef.h>

int accept_connection(int server_fd) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        return -1;
    }
    
    // Set non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    }
    
    return client_fd;
}