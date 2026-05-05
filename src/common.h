#ifndef COMMON_H
#define COMMON_H

#include "event/ringbuf.h"
#include <pthread.h>

#define PORT 8888
#define MAX_EVENTS 64

typedef struct client_ctx_t
{
    int fd;
    ringbuf_t ringbuf;
    pthread_mutex_t lock;
}client_ctx_t;


#endif