
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>     // strcasecmp
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>

#include "wordleLib.h"

#define SIZE sizeof(struct sockaddr_in)
#define DEFAULT_PORT 33651




// print a guess as colored tiles based on the server's G/Y/X feedback
static void show_result(const char *guess, const char *fb)
{
  for (int i = 0; i < WORD_LEN; i++)
    {
      const char *color = (fb[i] == 'G') ? "\033[42;30m"     // green
                        : (fb[i] == 'Y') ? "\033[43;30m"     // yellow
                        :                  "\033[100;97m";   // gray
      printf("%s %c \033[0m", color, toupper((unsigned char)guess[i]));
    }
  printf("\n");
}

// feedback must be exactly WORD_LEN chars of G / Y / X
static int valid_feedback(const char *fb)
{
  if (strlen(fb) != WORD_LEN)
    return 0;
  for (int i = 0; i < WORD_LEN; i++)
    if (fb[i] != 'G' && fb[i] != 'Y' && fb[i] != 'X')
      return 0;
  return 1;
}

// returns 1 = win, 0 = loss, -1 = error / quit
static int play_wordle(int sockfd)
{
  char input[MAX_MESSAGE];
  char reply[MAX_MESSAGE];
  char guess[WORD_BUF];
  int attempts = 0;

  printf("Guess the %d-letter word. You have %d tries. Type 'quit' to exit.\n",
         WORD_LEN, MAX_GUESSES);

  while (attempts < MAX_GUESSES)
    {
      printf("Guess %d/%d: ", attempts + 1, MAX_GUESSES);
      fflush(stdout);

      if (fgets(input, sizeof(input), stdin) == NULL)
        {
          send_message(sockfd, MSG_QUIT);
          return -1;
        }

      // discard leftover input if the line was longer than the buffer
      if (strchr(input, '\n') == NULL)
        {
          int c;
          while ((c = getchar()) != '\n' && c != EOF)
            ;
        }
      strip_newline(input);

      if (strcasecmp(input, "quit") == 0)
        {
          send_message(sockfd, MSG_QUIT);
          return -1;
        }

      if (!validate_word(input))
        {
          printf("Enter exactly %d letters (a-z).\n", WORD_LEN);
          continue;
        }

      normalize_word(input);
      strcpy(guess, input);

      char outbuf[WORD_BUF + 1];
      snprintf(outbuf, sizeof(outbuf), "%s\n", guess);
      if (send_message(sockfd, outbuf) == -1)
        return -1;

      int n = receive_message(sockfd, reply, sizeof(reply));
      if (n <= 0)
        {
          fprintf(stderr, "Server closed the connection.\n");
          return -1;
        }
      strip_newline(reply);

      if (strcmp(reply, "INVALID") == 0)
        {
          printf("Not in the word list, try again.\n");
          continue;                 // doesn't use up an attempt
        }

      if (!valid_feedback(reply))
        {
          fprintf(stderr, "Unexpected server reply: %s\n", reply);
          return -1;
        }

      attempts++;
      show_result(guess, reply);

      if (strcmp(reply, "GGGGG") == 0)
        return 1;
    }

  return 0;
}



static int connect_to_server(const char *ip, int base_port)
{
  for (int i = 0; i < MAX_PLAYERS; i++)
    {
       int port = base_port + i;
       if (port > 65535)
	  break;

       struct sockaddr_in addr;
       memset(&addr, 0, sizeof(addr));
       addr.sin_family = AF_INET;
       addr.sin_port = htons((unsigned short)port);
       addr.sin_addr.s_addr = inet_addr(ip);

       int fd = socket(AF_INET, SOCK_STREAM, 0);
       if (fd == -1)
	  {
	    perror("Socket call failed");
	    return -1;
	  }

       struct timeval tv = {5, 0};
       setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
       setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


       printf("Trying port %d...\n", port);
       if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	  {
	    printf("   port %d: %s\n", port, strerror(errno));
	    close_connection(fd);
	    continue;
	  }

       char buf[MAX_MESSAGE];
       if (receive_message(fd, buf, sizeof(buf)) <= 0)
	  {
	     printf("   port %d: no greeting\n", port);
	     close_connection(fd);
	     continue;
	  }

       strip_newline(buf);

       if (strcmp(buf, "BUSY") == 0)
	  {
            printf("   port %d: busy, trying next\n", port);
	    close_connection(fd);
	    continue;
          }

       if (command_type(buf) == CMD_HELLO)
	 {
            struct timeval none = {0, 0};
	    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &none, sizeof(none));
	    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &none, sizeof(none));
	    printf("Connected on port %d\n", port);
	    return fd;

	 }

       close_connection(fd);
     }
   return -1;
 }

int main(int argc, char *argv[])
{

  printf("Client machine started\n");
  fflush(stdout);

  int sockfd;
  long port = DEFAULT_PORT;
  char answer[32];
  struct sockaddr_in server = {0};

  signal(SIGPIPE, SIG_IGN);   // send() returns -1 instead of killing the program

  if (argc < 2 || argc > 3)
    {
      fprintf(stderr, "Usage: %s <IPv4-address> [port]\n", argv[0]);
      return EXIT_FAILURE;
    }

  // convert and store the server's IP address
  server.sin_addr.s_addr = inet_addr(argv[1]);
  if (server.sin_addr.s_addr == INADDR_NONE)
    {
      fprintf(stderr, "Invalid IPv4 address: %s\n", argv[1]);
      return EXIT_FAILURE;
    }

  if (argc == 3)
    {
      char *end;
      errno = 0;
      port = strtol(argv[2], &end, 10);
      if (!isdigit((unsigned char)argv[2][0]) || *end != '\0' ||
          errno == ERANGE || port < 1024 || port > 65535)
        {
          fprintf(stderr, "Port must be a number from 1024 to 65535.\n");
          return EXIT_FAILURE;
        }
    }

  sockfd = connect_to_server(argv[1], (int)port);
  if (sockfd < 0)
     {
       fprintf(stderr, "Could not connect to any server port (%ld-%ld).\n",
	       port, port + MAX_PLAYERS - 1);
       return EXIT_FAILURE;
     }

  printf("Wordle Game\n");
  printf("Start?(y/N) ");
  fflush(stdout);

  if (fgets(answer, sizeof(answer), stdin) == NULL)
    {
      send_message(sockfd, MSG_QUIT);
      close_connection(sockfd);
      return EXIT_SUCCESS;
    }

  if (answer[0] == 'n' || answer[0] == 'N')
    {
      send_message(sockfd, MSG_QUIT);
      close_connection(sockfd);
      printf("Connection closed.\n");
      return EXIT_SUCCESS;
    }

  if (answer[0] == 'y' || answer[0] == 'Y')
    {
      while (1)
        {
          if (send_message(sockfd, MSG_READY) == -1)
            break;

          int result = play_wordle(sockfd);
          if (result == 1)
            printf("You won!\n");
          else if (result == 0)
            printf("Out of guesses. Better luck next time!\n");
          else
            break;                    // player typed quit, or an error occurred

          // ask whether to play again
          int again = 0;
          while (1)
            {
              printf("Play again? (y/N) ");
              fflush(stdout);
              if (fgets(answer, sizeof(answer), stdin) == NULL)
                break;
              if (answer[0] == 'y' || answer[0] == 'Y')
                {
                  again = 1;
                  break;
                }
              if (answer[0] == 'n' || answer[0] == 'N' || answer[0] == '\n')
                break;
              printf("Please enter y or n.\n");
            }

          if (!again)
            {
              send_message(sockfd, MSG_QUIT);
              break;
            }
        }
    }

  close_connection(sockfd);
  printf("Connection closed.\n");
  return EXIT_SUCCESS;
}  // end main
