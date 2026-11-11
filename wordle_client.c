#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define SIZE sizeof(struct sockaddr_in)
#define DEFAULT_PORT 7000



int main(int argc, char *argv[])
{
  int sockfd;
  long port = DEFAULT_PORT;
  char answer[32];
  struct sockaddr_in server = {0};

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
          errno == ERANGE || port < 1 || port > 65535)
        {
          fprintf(stderr, "Port must be a number from 1 to 65535.\n");
          return EXIT_FAILURE;
        }
    }

  server.sin_family = AF_INET;
  server.sin_port = htons((unsigned short)port);
  
  // set up the transport end point
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror("socket call failed");
      exit(-1);
    }   // end if

  // connect the socket to the server's address
  if (connect(sockfd, (struct sockaddr *)&server, SIZE) == -1)
    {
      perror("connect call failed");
      close(sockfd);
      exit(-1);
    }   // end if

  printf("Wordle Game\n");
  printf("Start?(y/N) ");
  fflush(stdout);

  if (fgets(answer, sizeof(answer), stdin) == NULL)
    {
      close(sockfd);
      return EXIT_SUCCESS;
    }

  if (answer[0] == 'n' || answer[0] == 'N')
    {
      close(sockfd);
      printf("Connection closed.\n");
      return EXIT_SUCCESS;
    }

  if (answer[0] == 'y' || answer[0] == 'Y')
    {
      // Add the Wordle game here.
    }

  close(sockfd);
  return EXIT_SUCCESS;
}  // end main
