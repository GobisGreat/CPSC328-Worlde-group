//Authors: Gabe Segaline IT, Ryan Doan CS, Hunter Phillips CS
//Professor: Walther
//Filename: server.c
//Programming language used: C
//Course: CPSC328
//Creation date: 11/10/26
//Due date: 11/23/26
//Project: Wordle group Project
//Purpose: To demonstrate the understanding of creation of setting up a server with a persistent connection, allowing multiple users to connect at one time.



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "wordleLib.h"
//default port assigned below
#define DEFAULT_PORT 44389 
#define DEFAULT_WORD_FILE "valid-wordle-words.txt"
//The words list is 14,855 words long for the 5 letters A-Z
#define MAX_WORDS 14855

static char bank[MAX_WORDS][WORD_BUF];
static int  bank_count = 0;


//Function name: cmp_words
//Description: This functions purpose is to serve the functionality for the binary search, so that the bank can be alphabetically sorted for word_in_bank
//Parameters: a pointer for first word being compared, then b pointer for secind word being compared.
//return values: Returns negative number if a comes before b, 0 if the words are equal, and positive if a comes after b


static int cmp_words(const void *a, const void *b)
{
	return strcmp((const char *)a, (const char *)b);
} // end cmp_words


//Function name: word_in_bank
//Description: This function checks whether the guess is a real word
//and is done by performing a binary search in the word bank.
//Parameters: word, word_bank, and word_count
//return value: returns 1 if word is in the bank and 0 if the word is not.



int word_in_bank(const char *word, char word_bank[][6], int word_count){
	// im gonna use a binary search for this cuz the list is like 14 thousand or so long
	int low = 0;
	int high = word_count - 1;

	while(low <= high){
		int mid = (low + high) / 2;
		int comparison = strcmp(word, word_bank[mid]); // 0 if its the word, less than 0 if its in low half > 0 if its high half

		if (comparison == 0){
			return 1; // word found 
		}

		else if (comparison < 0)
		{
			high = mid - 1; // search low
		} // end elif
		else {
			low = mid + 1; // search high
			}
		
	} // end while
		return 0; // word not found
	} // end word in bank



//Function name: load_words
//Description: This function reads 5 letter words from the file into the word bank, skipping any blank lines
//which then sorts the bank so that the word_in_bank function works with the binary search.
//Parameters: path
//return values: 0 on success, -1 if the file cannot open

// load 5-letter words from a file, then sort so word_in_bank's binary search works
static int load_words(const char *path)
{
	FILE *fp = fopen(path, "r");
	if (!fp) {
		perror("fopen word file");
		return -1;
	} // end if

	char line[MAX_MESSAGE];
	while (bank_count < MAX_WORDS && fgets(line, sizeof(line), fp)) {
		strip_newline(line);
		if (!validate_word(line))
			continue;                  // skip blank or malformed lines
		normalize_word(line);
		strcpy(bank[bank_count++], line);
	} // end while
	fclose(fp);

	if (bank_count == 0) {
		fprintf(stderr, "Word file is empty or has no valid words.\n");
		return -1;
	} // end if

	qsort(bank, bank_count, WORD_BUF, cmp_words);
	return 0;
} // end load_words


//Function name:score_guess
//Description: Compares guess to secret word and builds worlde feedback string
//Parameters: secret - the hidden word
//return value: None



// fb gets WORD_LEN chars of G / Y / X, null-terminated
static void score_guess(const char *secret, const char *guess, char *fb)
{
	int remaining[26] = {0};

	// pass 1: greens, and count the secret letters not matched exactly
	for (int i = 0; i < WORD_LEN; i++) {
		if (guess[i] == secret[i]) {
			fb[i] = 'G';
		} else {
			fb[i] = 'X';
			remaining[secret[i] - 'a']++;
		} // end if
	} // end for

	// pass 2: yellows, only while unmatched copies of that letter remain
	for (int i = 0; i < WORD_LEN; i++) {
		if (fb[i] == 'X' && remaining[guess[i] - 'a'] > 0) {
			fb[i] = 'Y';
			remaining[guess[i] - 'a']--;
		} // end if
	} // end for

	fb[WORD_LEN] = '\0';
} // end score_guess


//Function name: play_game
//Description: runs the full wordle game with the client, picks random word, loops if the client wants to play again
//Parameters: client_fd - socket connected to client
//return value: None


static void play_game(int client_fd, int slot)
{
	const char *secret = bank[rand() % bank_count];
	char buf[MAX_MESSAGE];
	char fb[WORD_BUF + 1];
	int attempts = 0;

	printf("(Slot %d) Secret word: %s\n", slot, secret);   // debug: remove before turning in

	while (attempts < MAX_GUESSES) {
		int n = receive_message(client_fd, buf, sizeof(buf));
		if (n <= 0) {
			printf("(Slot %d) Client disconnected.\n", slot);
			return;
		} // end if
		strip_newline(buf);

		if (command_type(buf) == CMD_QUIT) {
			printf("(Slot %d) Client quit.\n", slot);
			return;
		} // end if

		normalize_word(buf);

		if (!validate_word(buf) || !word_in_bank(buf, bank, bank_count)) {
			if (send_message(client_fd, MSG_INVALID) == -1)
				return;
			continue;                  // doesn't use up an attempt
		} // end if

		attempts++;
		score_guess(secret, buf, fb);
		fb[WORD_LEN]     = '\n';
		fb[WORD_LEN + 1] = '\0';
		if (send_message(client_fd, fb) == -1)
			return;

		if (strncmp(fb, "GGGGG", WORD_LEN) == 0) {
			printf("(Slot %d) Client won in %d guesses.\n", slot, attempts);
			return;
		} // end if
	} // end while

	printf("(Slot %d) Client ran out of guesses.\n", slot);
} // end play_game

//Function name: make_listener
//Description: This creates a TCP socket, binds,  and listens. (The accept call is later down in main)
//Note: setsockopt is used here because we kept having issues with ports already in use so this allows us to reuse those ports.
//Parameters: port
//returns -1 if any errors are present socket creation and with binding and listening. Returns the sockets file descriptor otherwise



static int make_listener(int port)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
     perror("socket");
     return -1;
  }

  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //This setsockopt allows us to set the options for the socket
							      								 //we can reuse this port if the server restarts so that we dont
							      								 //get a binding error to that specific port
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((unsigned short)port);


  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      fprintf(stderr, "bind port %d: ", port);
      perror("");
      close(fd);
      return -1;
  }


  if (listen(fd, 5) < 0) {
       perror("listen");
       close(fd);
       return -1;
  }
  return fd;
}

//Function name: handle_client
//Description: this function allows to be ran in the forked child process, so that one client connection is its own entity and can run with atomicity.
//Parameters: client_fd
//return values: none

static void handle_client(int client_fd, int slot)
{
  if (send_message(client_fd, MSG_HELLO) == -1)
     return;


  char buf[MAX_MESSAGE];
  while (receive_message(client_fd, buf, sizeof(buf)) > 0) {
     strip_newline(buf);
     int cmd = command_type(buf);

     if (cmd == CMD_READY) {
        play_game(client_fd, slot);
     } else if (cmd == CMD_QUIT) {
           printf("(Slot %d) Client quit.\n", slot);
           break;
     } else {
           printf("(Slot %d) Unknown command from client.\n", slot);
           break;
     }//end if
  } //end while

} //end handle client

//function name:
//description: This function, main, starts the whole server and promotes a persistent connection
//Parameters: argc, argv
//return values: 0 on normal exit, 1 on a startup issue



int main(int argc, char *argv[])
{
	long port = DEFAULT_PORT;
	const char *word_file = DEFAULT_WORD_FILE;

        signal(SIGPIPE, SIG_IGN);

	//usage error for making sure the word file is known and port is known aswell.
        if (argc > 3) {
	    fprintf(stderr, "Usage: %s [port] [wordfile]\n", argv[0]);
	    return 1;
        }

	//the argument for the port cannot be below 1 or over 65535.
	if (argc >= 2) {
            char *end;
	    errno = 0;
	    port = strtol(argv[1], &end, 10);
	    if (!isdigit((unsigned char)argv[1][0]) || *end != '\0' ||
		errno == ERANGE || port < 1024 || port > 65535) {
	        fprintf(stderr, "Port must be a number from 1024 to 65535.\n");
		return 1;
	    }
	}


        if (argc == 3)
	    word_file = argv[2];
	if (load_words(word_file) == -1)
	    return 1;
        printf("Loaded %d words.\n", bank_count);


	//this is usage for ports assigned to server to listen that are below 65535;
	//however the iterated ports may go over 65535, so we need to make sure when
	//its iterated that it doesnt go over and stays under that 65535, so for 10
	//concurrent users technically we can only use upt to 65525 so it iterates up
	//to 65535 and doesn't tip over.
	if (port + (MAX_PLAYERS - 1) > 65535) {
            fprintf(stderr, "Base port too high: need %d consecutive ports.\n", MAX_PLAYERS);
	    return 1;
        }

        int listen_fd[MAX_PLAYERS];
        pid_t slot_pid[MAX_PLAYERS];


        for (int i = 0; i < MAX_PLAYERS; i++) {
             slot_pid[i] = 0;
	     listen_fd[i] = make_listener((int)port + i);
             if (listen_fd[i] < 0)
     		 return 1;
             printf("Slot %d listening on port %ld\n", i + 1, port + i);
        }



        while (1) {
	   pid_t done;
	   while ((done = waitpid(-1, NULL, WNOHANG)) > 0)  //The reason as to why WNOHANG is in here is because the 
	        for (int i = 0; i < MAX_PLAYERS; i++)       //server needs to accept new clients, and if this wasn't included,
                     if (slot_pid[i] == done) {         //the server would freeze until the child process finished.
		         slot_pid[i] = 0;						//This is included in sys/wait.h and allows new connections of clients without stopping the server 
			 printf("Slot %d (port %ld) is free again.\n", i + 1, port + i);
		     }

           fd_set rfds;
	   FD_ZERO(&rfds);
	   int maxfd = -1;
	   for (int i = 0; i < MAX_PLAYERS; i++) {
               FD_SET(listen_fd[i], &rfds);
	       if (listen_fd[i] > maxfd)
	           maxfd = listen_fd[i];
	   }

           struct timeval tv = {1, 0};
	   int ready = select(maxfd + 1, &rfds, NULL, NULL, &tv);
           if (ready < 0) {
	      if (errno == EINTR)
		  continue;
	      perror("select");
	      break;
	   }

           for (int i = 0; i < MAX_PLAYERS; i++) {
               if (!FD_ISSET(listen_fd[i], &rfds))
		   continue;

	       struct sockaddr_in client_addr;
	       socklen_t addr_len = sizeof(client_addr);
	       int client_fd = accept(listen_fd[i], (struct sockaddr *)&client_addr, &addr_len);
	       if (client_fd < 0) {
		   perror("accept");
		   continue;
	       }


	       while ((done = waitpid(-1, NULL, WNOHANG)) > 0)
		   for (int j = 0; j < MAX_PLAYERS; j++)
		       if (slot_pid[j] == done)
			   slot_pid[j] = 0;

               if (slot_pid[i] != 0) {
                   send_message(client_fd, MSG_BUSY); //client will try the next port num
		   close_connection(client_fd);
		   continue;
               }


	       printf("Client %s connected on port %ld (slot %d)\n",
		       inet_ntoa(client_addr.sin_addr), port + 1, i + 1);

	       pid_t pid = fork();
	       if (pid < 0) {
		  perror("fork");
		  close_connection(client_fd);
		  continue;
               }

	       if (pid == 0) {
		   for (int j = 0; j < MAX_PLAYERS; j++)
		        close_connection(listen_fd[j]);
		   srand((unsigned)time(NULL) ^ (unsigned)getpid());
		   handle_client(client_fd, i + 1);
		   close_connection(client_fd);
                   printf("Client session ended.\n");
		   exit(0);
	       }


	       slot_pid[i] = pid;
	       close_connection(client_fd);
	   }
	}

	for (int i = 0; i < MAX_PLAYERS; i++)
	     close_connection(listen_fd[i]);
	return 0;
} //end main































































