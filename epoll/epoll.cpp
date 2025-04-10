#include "../inc/epoll.hpp"

static int kq = -1;

int epoll_create1(int flags) {
    (void)flags;
    kq = kqueue();
    if (kq == -1)
        return -1;
    return kq;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    (void)epfd;
    struct kevent ke;
    int filter = (event->events & EPOLLIN) ? EVFILT_READ : EVFILT_WRITE;

    switch (op) {
        case EPOLL_CTL_ADD:
        case EPOLL_CTL_MOD:
            EV_SET(&ke, fd, filter, EV_ADD | EV_ENABLE, 0, 0,
                   reinterpret_cast<void*>(static_cast<intptr_t>(event->data.fd)));
            break;
        case EPOLL_CTL_DEL:
            EV_SET(&ke, fd, filter, EV_DELETE, 0, 0, NULL);
            break;
        default:
            return -1;
    }

    if (kevent(kq, &ke, 1, NULL, 0, NULL) == -1)
        return -1;

    return 0;
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    (void)epfd;
    struct timespec ts;
    struct timespec* pts = NULL;

    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
        pts = &ts;
    }

    struct kevent kevents[MAX_EVENTS];
    int n = kevent(kq, NULL, 0, kevents, maxevents, pts);
    if (n == -1)
        return -1;

    for (int i = 0; i < n; ++i) {
        events[i].events = 0;
        if (kevents[i].filter == EVFILT_READ)
            events[i].events |= EPOLLIN;
        if (kevents[i].filter == EVFILT_WRITE)
            events[i].events |= EPOLLOUT;
        events[i].data.fd = static_cast<int>(reinterpret_cast<intptr_t>(kevents[i].udata));
    }

    return n;
}
