#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <stdint.h> // for intptr_t

#define EPOLLIN  0x001
#define EPOLLOUT 0x004
#define EPOLLERR 0x008
#define EPOLLHUP 0x010
#define EPOLLET  1u << 31

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

#define MAX_EVENTS 64

struct epoll_event {
    uint32_t events;
    union epoll_data {
        int fd;
    } data;
};

int epoll_create1(int flags);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

#endif
