#include "bind_socket.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

int bind_socket(int sockfd, const char* socket_path) {
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    return bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
}