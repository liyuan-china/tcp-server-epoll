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
#include <worker/threadpool.h>


/**
 * 回显结构体
 */
typedef struct echo_task_t
{
    int fd;
    char buf[1024*16];
    ssize_t len;
    int epfd;

}echo_task_t;

/**
 * 工作线程执行的回显任务
 * @param arg   
 */
static void echo_worker(void *arg){
    echo_task_t *task = (echo_task_t *)arg;

    ssize_t written = 0;
    while (written < task->len)
    {
        ssize_t ret = write(task->fd, task->buf + written, task->len - written);
        if (ret == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(1000);
                continue;
                /* code */
            }
            perror("write in worker");
            break;
            /* code */
        }
        written += ret;
        /* code */
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = task->fd;
    //close(task->fd);
    epoll_ctl(task->epfd, EPOLL_CTL_MOD, task->fd, &ev);
    free(task);
    

}

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
void handle_client_data(int client_fd, int epfd, threadpool_t *pool)
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
        echo_task_t *task = (echo_task_t *)malloc(sizeof(echo_task_t));
        if (task == NULL)
        {
            close(client_fd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
            return;
            /* code */
        }
        task->fd = client_fd;
        memcpy(task->buf, full_buf, total);
        task->len = total;
        task->epfd = epfd;

        threadpool_task(pool, echo_worker, task);

    } else if (eof) {
        printf("Client %d closed.\n", client_fd);
        close(client_fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);        
    }
}