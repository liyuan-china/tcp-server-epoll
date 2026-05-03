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
#include <common.h>


/**
 * 将文件描述符设置为非阻塞模式
 * @param fd 需要设置的文件描述符
 * @param 成功返回0，失败返回-1
 * @return 无返回值(void)
 */
int set_nonblocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        printf("fcntl F_GETFL error.\n");
        /* code */
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
      printf("fcntl F_SETFL error.\n");

    return flags;
    
}

/**
 * 创建并绑定监听socket
 * @return 成功返回监听socket的文件描述符，失败返回-1
 */
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