#ifndef BIND_SOCKET_H
#define BIND_SOCKET_H

/**
 * Binds socket to Unix domain socket path
 * @param sockfd Socket file descriptor
 * @param socket_path Path to bind to
 * @return 0 on success, -1 on error
 */
int bind_socket(int sockfd, const char* socket_path);

#endif