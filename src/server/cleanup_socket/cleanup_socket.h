#ifndef CLEANUP_SOCKET_H
#define CLEANUP_SOCKET_H

/**
 * Cleans up socket resources
 * @param sockfd Socket file descriptor
 * @param socket_path Socket file path to unlink
 */
void cleanup_socket(int sockfd, const char* socket_path);

#endif