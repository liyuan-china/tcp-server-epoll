#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

void handle_accept(int listenfd, int epfd);
void handle_client_data(int client_fd, int epfd);


#endif