#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "worker/threadpool.h"

void handle_accept(int listenfd, int epfd);
void handle_client_data(client_ctx_t *ctx, int epfd, threadpool_t *pool);


#endif