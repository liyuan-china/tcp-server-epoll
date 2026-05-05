CC	=	gcc
CFLAGS	=	-Wall -pthread -Isrc
TARGET	=	build/server

SRCS	=	src/main.c	\
			src/net/socket_util.c	\
			src/event/epoll_util.c	\
			src/event/ringbuf.c	\
			src/event/frame.c	\
			src/handler/client_handler.c	\
			src/worker/threadpool.c

$(TARGET): $(SRCS)
		mkdir -p build
		$(CC) $(CFLAGS) $(SRCS)	-o	$@

clean:
		rm -rf build/server