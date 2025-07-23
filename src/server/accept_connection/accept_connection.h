#ifndef ACCEPT_CONNECTION_H
#define ACCEPT_CONNECTION_H

/**
 * Accepts new client connection
 * @param server_fd Server socket file descriptor
 * @return Client file descriptor or -1 on error
 */
int accept_connection(int server_fd);

#endif