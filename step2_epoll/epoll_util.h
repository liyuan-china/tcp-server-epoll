#ifndef EPOLL_UTILL_H
#define EPOLL_UTILL_H

int create_epoll(int listenfd);
void event_loop(int epfd, int listen_fd);

#endif