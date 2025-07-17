#include "transport_utils.h"
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

size_t process_received_buffer(char* buffer, size_t received) {
    if (!buffer || received == 0) return 0;
    
    buffer[received] = '\0';
    
    // Remove trailing newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
        len--;
    }
    
    return len;
}

bool is_receive_timeout_or_error(ssize_t result) {
    return result <= 0;
}

int standard_buffer_processing(char* buffer, size_t buffer_size, ssize_t received) {
    if (!buffer || buffer_size == 0) return -1;
    
    if (is_receive_timeout_or_error(received)) {
        return -1;
    }
    
    if ((size_t)received >= buffer_size) {
        return -1; // Buffer overflow
    }
    
    process_received_buffer(buffer, (size_t)received);
    return 0;
}

int create_tcp_socket(void) {
    return socket(AF_INET, SOCK_STREAM, 0);
}

int create_udp_socket(void) {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

int setup_socket_address(struct sockaddr_in* addr, const char* host, int port) {
    if (!addr) return -1;
    
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    
    if (host) {
        if (inet_pton(AF_INET, host, &addr->sin_addr) <= 0) {
            return -1;
        }
    } else {
        addr->sin_addr.s_addr = INADDR_ANY;
    }
    
    return 0;
}

int setup_server_socket(int socket_fd, struct sockaddr_in* addr, bool is_tcp) {
    if (socket_fd < 0 || !addr) return -1;
    
    // Set SO_REUSEADDR
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        return -1;
    }
    
    // Bind socket
    if (bind(socket_fd, (struct sockaddr*)addr, sizeof(*addr)) < 0) {
        return -1;
    }
    
    // Listen for TCP sockets
    if (is_tcp) {
        if (listen(socket_fd, 1) < 0) {
            return -1;
        }
    }
    
    return 0;
}

int connect_socket(int socket_fd, struct sockaddr_in* addr) {
    if (socket_fd < 0 || !addr) return -1;
    
    return connect(socket_fd, (struct sockaddr*)addr, sizeof(*addr));
}

int set_socket_nonblocking(int socket_fd) {
    if (socket_fd < 0) return -1;
    
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags < 0) return -1;
    
    return fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}

void close_socket(int socket_fd) {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

void setup_select_timeout(int timeout_ms, struct timeval* tv) {
    if (!tv) return;
    
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms % 1000) * 1000;
}

int socket_select_read(int socket_fd, int timeout_ms) {
    if (socket_fd < 0) return -1;
    
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    
    struct timeval tv;
    setup_select_timeout(timeout_ms, &tv);
    
    return select(socket_fd + 1, &read_fds, NULL, NULL, &tv);
}