#include "setup_epoll.h"
#include <sys/epoll.h>

int setup_epoll(void) {
    return epoll_create1(EPOLL_CLOEXEC);
}