CC	=	gcc
CFLAGS	=	-Wall -pthread -Isrc
TARGET	=	build/server

SRCS	=	src/main.c	\
			src/net/socket_util.c	\
			src/event/epoll_util.c	\
			src/handler/client_handler.c	\
			src/worker/threadpool.c

$(TARGET):
		mkdir -p build
		$(CC) $(CFLAGS) $(SRCS)	-o	$@

clean:
		rm -rf build/server