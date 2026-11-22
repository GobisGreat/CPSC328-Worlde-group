#ifndef WORDLELIB_H
#define WORDLELIB_H

#define MAX_MESSAGE 512
#define WORD_LEN    5
#define WORD_BUF    (WORD_LEN + 1)
#define MAX_GUESSES 6

#define CMD_UNKNOWN 0
#define CMD_QUIT    1
#define CMD_HELLO   2
#define CMD_READY   3
#define MSG_BUSY    "BUSY\n"
#define MAX_PLAYERS 10


#define MSG_HELLO   "HELLO\n"
#define MSG_READY   "READY\n"
#define MSG_QUIT    "QUIT\n"
#define MSG_INVALID "INVALID\n"

int  send_message(int socket_fd, const char *message);
int  receive_message(int socket_fd, char *buffer, int buffer_size);
void close_connection(int socket_fd);

void strip_newline(char *s);
int  validate_word(const char *word);
void normalize_word(char *word);
int  command_type(const char *message);



#endif
