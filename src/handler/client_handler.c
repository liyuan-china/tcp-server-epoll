#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "net/socket_util.h"
#include "client_handler.h"


/**
 * 处理新连接(ET模式下许循环accept直至EAGAIN)
 * @param listenfd 监听套接字，由create_bind()创建
 * @param epfd     epoll实例的文件描述符，用于注册新连接的客户端client_fd
 * @return 无返回值 (void)
 */
void handle_accept(int listenfd, int epfd){
    while (1)
    {
        int client_fd = accept(listenfd, NULL, NULL);
        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            printf("accept failed.\n");
            break;
            /* code */
        }
        set_nonblocking(client_fd);
        struct epoll_event client_ev;
        client_ev.events = EPOLLIN | EPOLLET;
        client_ev.data.fd = client_fd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0)
        {
            printf("epoll_ctl add client failed.\n");
            close(client_fd);
            /* code */
        }
        else
        {
            printf("New client connected:%d\n", client_fd);
        }
    }
}

/**
 * 处理客户端数据读取与回显(ET模式下循环读至EAGAIN,然后一次性回写)
 * @param client_fd 客户端套接字的文件描述符
 * @param epfd      epoll实例的文件描述符,出错或关闭连接时用于从epoll中移除该fd
 * @return 无返回值(void)
 */
//处理客户端数据（echo）
void handle_client_data(int client_fd, int epfd)
{
    char full_buf[1024 * 16];
    ssize_t total = 0;
    int eof = 0;
    while (1)
    {
        ssize_t n = read(client_fd, full_buf + total, sizeof(full_buf) - total);
        // printf("Received %d bytes from client %d\n", n, client_fd);
        // printf("read %d bytes\n", n);
        if (n == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
                /* code */
            }
            else
            {
                perror("read.");
                close(client_fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
                return;
            }
        }
        else if (n == 0)
        {
            // 客户端关闭连接
            eof = 1;
            break; // 没有更多数据可读
        }
        else
        {
            total += n;
            printf("read %zd bytes, total %zd\n", n, total);
        }
    }
    if (total > 0)
    {
        // 数据已完整接收，现在一次性回写
        printf("Total read = %zd, now echoing back.\n", total);
        ssize_t total_writer = 0;
        while (total_writer < total)
        {
            ssize_t ret = write(client_fd, full_buf + total_writer, total - total_writer);
            if (ret == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    usleep(1000);
                    continue;
                    /* code */
                }
                printf("write failed.\n");
                break;
                /* code */
            }
            total_writer += ret;
            /* code */
        }
        if (total_writer == total)
        {
            printf("Echo to %d: %.*s", client_fd, (int)total, full_buf);
            /* code */
        }
        else
        {
            printf("Imcomplete echo to %d\n", client_fd);
        }
    }
    if (eof)
    {
        printf("Client %d closed.\n", client_fd);
        close(client_fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
        /* code */
    }
}