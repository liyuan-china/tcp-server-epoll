#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <common.h>
#include "net/socket_util.h"
#include "event/epoll_util.h"
#include "handler/client_handler.h"

/**
 * 高并发epoll echo服务器入口
 * 流程：创建监听socket→设置非阻塞→创建epoll→进入事件循环
 * @return 0 正常退出，1 初始化失败
 */
int main(){
    int listen_fd = create_bind();
    if (listen_fd < 0)
    {
        fprintf(stderr, "Failed to create and bind socket.\n");
        return 1;
        /* code */
    }

    set_nonblocking(listen_fd);

    int num_threads = 0;
    const char *env_threads = getenv("THREAD_NUM");
    if (env_threads)
    {
        num_threads = atoi(env_threads);
        if (num_threads <= 0)
        {
            num_threads = sysconf(_SC_NPROCESSORS_ONLN);
        }
    }else{
        num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }
    
    // 创建监听 fd 的context
    client_ctx_t *listen_ctx = malloc(sizeof(client_ctx_t));
    listen_ctx->fd = listen_fd;
    memset(&listen_ctx->ringbuf, 0, sizeof(ringbuf_t));
    memset(&listen_ctx->lock, 0, sizeof(pthread_mutex_t));

    int epfd = create_epoll(listen_fd, listen_ctx);
    if (epfd < 0)
    {
        free(listen_ctx);
        close(listen_fd);
        return 1;
    }
    printf("Server listening on port %d (epoll ET mode)\n", PORT);

    threadpool_t *pool = threadpool_create(num_threads, 100);
    event_loop(epfd, listen_fd, pool);
    threadpool_destroy(pool);

    close(listen_fd);
    close(epfd);
    
    
    return 0;
}