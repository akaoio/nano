#ifndef LISTEN_SOCKET_H
#define LISTEN_SOCKET_H

/**
 * Sets socket to listen mode
 * @param sockfd Socket file descriptor
 * @param backlog Maximum pending connections
 * @return 0 on success, -1 on error
 */
int listen_socket(int sockfd, int backlog);

#endif