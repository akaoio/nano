#include "create_socket.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int create_socket(const char* socket_path) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    // Remove existing socket file if it exists
    unlink(socket_path);
    
    return sockfd;
}