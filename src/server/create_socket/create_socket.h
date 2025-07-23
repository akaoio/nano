#ifndef CREATE_SOCKET_H
#define CREATE_SOCKET_H

/**
 * Creates a Unix domain socket
 * @param socket_path Path to the socket file
 * @return Socket file descriptor or -1 on error
 */
int create_socket(const char* socket_path);

#endif