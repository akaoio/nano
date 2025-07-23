#ifndef SETUP_EPOLL_H
#define SETUP_EPOLL_H

/**
 * Initializes epoll for non-blocking I/O
 * @return epoll file descriptor or -1 on error
 */
int setup_epoll(void);

#endif