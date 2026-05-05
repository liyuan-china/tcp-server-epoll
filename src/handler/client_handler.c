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
#include "event/frame.h"


/**
 * 回显结构体
 */
typedef struct echo_task_t
{
    int fd;
    char *buf;
    ssize_t len;
    int epfd;

    client_ctx_t *ctx;
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
            }
            perror("write in worker");
            break;
        }
        written += ret;
    }

    // 重新激活 epoll 监听(边缘触发必须)
    if (task->ctx)
    {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = task->ctx;
        epoll_ctl(task->epfd, EPOLL_CTL_MOD, task->fd, &ev);
    }
    free(task->buf);
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
        }

        set_nonblocking(client_fd);
        client_ctx_t *ctx = malloc(sizeof(client_ctx_t));
        ctx->fd = client_fd;
        ringbuf_init(&ctx->ringbuf, 4096);
        pthread_mutex_init(&ctx->lock, NULL);

        struct epoll_event client_ev;
        client_ev.events = EPOLLIN | EPOLLET;
        client_ev.data.ptr = ctx;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0)
        {
            printf("epoll_ctl add client failed.\n");
            close(client_fd);
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
void handle_client_data(client_ctx_t *ctx, int epfd, threadpool_t *pool)
{
    char tmp[4096];

    // 1.读fd直到EAGAIN
    while (1)
    {
        ssize_t n = read(ctx->fd, tmp, sizeof(tmp));
        if (n > 0)
        {
            ringbuf_write(&ctx->ringbuf, tmp, n); 
        }
        else if (n == 0)
        {
            goto cleanup;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }

            goto cleanup;
        }
    }

    // 2.提取完整帧
    while (1)
    {
        char *payload = NULL;
        uint16_t payload_len = 0;

        int ret = frame_try_extract(&ctx->ringbuf, &payload, &payload_len);
        if (ret == 0)
        {
            printf("Recevied payload:%.*s\n", payload_len, payload);
            // 成功一帧→提交线程池
            echo_task_t *task = malloc(sizeof(echo_task_t));
            task->fd = ctx->fd;
            task->buf = payload;
            task->len = payload_len;
            task->epfd = epfd;
            threadpool_task(pool, echo_worker, task);
        }
        else if (ret == 1)
        {
            break;
        }
        else
        {
           goto cleanup;
        }
    }

    return;

cleanup:
    close(ctx->fd);
    epoll_ctl(epfd, EPOLL_CTL_DEL, ctx->fd, NULL);
    ringbuf_destroy(&ctx->ringbuf);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}