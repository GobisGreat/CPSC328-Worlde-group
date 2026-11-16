#ifndef WORDLELIB_H
#define WORDLELIB_H

#define MAX_MESSAGE 512


// decls
int send_message(int socket_fd, const char *message);
int receive_message(int socket_fd, char *buffer, int buffer_size);

void close_connection(int socket_fd);

#endif
