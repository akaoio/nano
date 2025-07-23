#include "listen_socket.h"
#include <sys/socket.h>

int listen_socket(int sockfd, int backlog) {
    return listen(sockfd, backlog);
}