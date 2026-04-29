#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include "common.h"
#include "epoll_util.h"
#include "client_handler.h"

/**
 * 创建epoll实例，并将监听socket加入epoll监听
 * @param listenfd 监听套接字
 * @return 成功返回epoll实例文件描述符，失败返回-1
 */
int create_epoll(int listenfd){
    int epfd = epoll_create1(0);
    if (epfd < 0)
    {
        printf("epoll_create1 error.\n");
        return -1;
    }

    //将监听加入到epoll
    struct epoll_event epev;
    epev.events = EPOLLIN | EPOLLET;//水平触发
    epev.data.fd = listenfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &epev) < 0)
    {
        printf("epoll_ctl add listen error.\n");
        return -1;
    }
    return epfd;

}


/**
 * epoll事件循环：持续等待时间，分发给不同的处理函数
 * @param epfd   epoll实例的文件描述符
 * @param listen_fd 监听套接字的文件描述符，用于区分新连接事件和数据事件
 * @return 无返回值(void)
 */
void event_loop(int epfd, int listen_fd){
    struct epoll_event events[MAX_EVENTS];
    while (1)
    {
        int nfd = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfd < 0)
        {
            if (errno == EINTR)
            {
                continue;//信号中断，继续等待
            }  
            printf("epoll wait...\n");
            break;
        }
        for (int i = 0; i < nfd; i++)
        {
            int event_fd = events[i].data.fd;
            if (event_fd == listen_fd)
            {
                handle_accept(listen_fd, epfd);        
            }else{
                handle_client_data(event_fd, epfd);
            }          
        }  
    }

}