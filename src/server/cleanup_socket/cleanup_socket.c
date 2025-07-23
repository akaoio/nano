#include "cleanup_socket.h"
#include <unistd.h>

void cleanup_socket(int sockfd, const char* socket_path) {
    if (sockfd >= 0) {
        close(sockfd);
    }
    if (socket_path) {
        unlink(socket_path);
    }
}