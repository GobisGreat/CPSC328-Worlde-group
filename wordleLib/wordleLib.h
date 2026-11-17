#ifndef WORDLELIB_H
#define WORDLELIB_H

#define MAX_MESSAGE 512

#define CMD_UNKNOWN 0
#define CMD_QUIT 1
#define CMD_HELLO 2
#define CMD_READY 3  // differntiate the different type of exits allowed in command_type, we can either use these or enum's up to u guys


// decls
int send_message(int socket_fd, const char *message);
int receive_message(int socket_fd, char *buffer, int buffer_size);


void close_connection(int socket_fd);

int validate_word(const char *word);
void normalize_word(char *word);
int command_type(const char *message);

int word_in_bank(const char *word, char word_bank[][6], int word_count);
#endif
