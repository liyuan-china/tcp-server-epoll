#ifndef EPOLL_UTILL_H
#define EPOLL_UTILL_H

#include "worker/threadpool.h"

int create_epoll(int listenfd);
void event_loop(int epfd, int listen_fd, threadpool_t *pool);

#endif