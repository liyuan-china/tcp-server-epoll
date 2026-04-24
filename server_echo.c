#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
//#include <sys/types.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8888
#define MAX_EVENTS 64

//设置文件描述符为非阻塞
void set_nonblocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        printf("fcntl F_GETFL error.\n");
        /* code */
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
      printf("fcntl F_SETFL error.\n");
    
}

//创建并绑定监听socket，返回listenfd
int create_bind(){
    // 1.创建套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        printf("listen error.\n");
        return -1;
        /* code */
    }

    //允许服务器重启后立即绑定到同一个端口，而不是被“地址已占用”的错误卡住
    int opt = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        printf("set scoketopt failed.\n");
        close(listenfd);
        return -1;

    }
    // 2.绑定地址和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("bind error.\n");
        close(listenfd);
        return -1;
        /* code */
    }
    //3.开始监听
    if (listen(listenfd, 128) < 0)
    {
        printf("listen error.\n");
        close(listenfd);
        return -1;
        /* code */
    }

    return listenfd;
}

// 创建epoll实例并将监听event_fd加入
int create_epoll(int listenfd){
    int epfd = epoll_create1(0);
    if (epfd < 0)
    {
        printf("epoll_create1 error.\n");
        return -1;
        /* code */
    }

    //将监听加入到epoll
    struct epoll_event epev;
    epev.events = EPOLLIN;//水平触发
    epev.data.fd = listenfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &epev) < 0)
    {
        printf("epoll_ctl add listen error.\n");
        return -1;
        /* code */
    }
    return epfd;

}

// 处理新连接
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
        client_ev.events = EPOLLIN;
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

//处理客户端数据（echo）
void handle_client_data(int client_fd, int epfd){
    char buf[1024];
    int n = read(client_fd, buf, sizeof(buf));
    if (n == 0)
    {
        printf("Client %d closed.\n", client_fd);
        close(client_fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
        /* code */
    }
    else if (n < 0)
    {
        printf("read failed.\n");
        epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
        /* code */
    }
    else
    {
        write(client_fd, buf, n);
        printf("Echo to %d: %s", client_fd, buf);
    }
}

// epoll事件
void event_loop(int epfd, int listen_fd){
    struct epoll_event events[MAX_EVENTS];
    while (1)
    {
        int nfd = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfd < 0)
        {
            printf("epoll wait...\n");
            break;
            /* code */
        }
        for (int i = 0; i < nfd; i++)
        {
            int event_fd = events[i].data.fd;
            if (event_fd == listen_fd)
            {
                handle_accept(listen_fd, epfd);        
                /* code */
            }else{
                handle_client_data(event_fd, epfd);
            }          
            /* code */
        }  
        /* code */
    }

}

int main(){
    int listen_fd = create_bind();
    if (listen_fd < 0)
    {
        fprintf(stderr, "Failed to create and bind socket.\n");
        return 1;
        /* code */
    }

    set_nonblocking(listen_fd);

    int epfd = create_epoll(listen_fd);
    if (epfd < 0)
    {
        close(listen_fd);
        return 1;
        /* code */
    }
    printf("Server listening on port %d (epoll LT mode)\n", PORT);

    event_loop(epfd, listen_fd);

    close(listen_fd);
    close(epfd);
    
    
    return 0;
}