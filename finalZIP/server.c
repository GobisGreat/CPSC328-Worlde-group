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
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#include "wordleLib.h"
//default port assigned below
#define DEFAULT_PORT 44389 
#define WORD_FILE "valid-wordle-words.txt"
//The words list is 14,855 words long for the 5 letters A-Z
#define MAX_WORDS 14855

static char bank[MAX_WORDS][WORD_BUF];
static int  bank_count = 0;


//Function name: word_in_bank
//Description: This function checks whether the guess is a real word
//and is done by performing a binary search in the word bank.
//The word file must already be in alphabetical order for this to work.
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
//Description: This function reads 5 letter words from the file into the word bank, skipping any blank lines.
//The file is already in alphabetical order, so no sorting is needed for the binary search in word_in_bank.
//Parameters: path
//return values: 0 on success, -1 if the file cannot open

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
//Description: runs one wordle game with the client and picks a random secret word.
//The client already lowercases and checks that each guess is 5 letters a-z, so the
//server only has to check that the guess is in the word bank.
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
	       if (n <= 0)
		     return;                    // client disconnected
	       strip_newline(buf);

	       if (command_type(buf) == CMD_QUIT)
		     return;

	       if (!word_in_bank(buf, bank, bank_count)) {
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
//Description: This creates a TCP socket, binds,  and listens. (The accept call is later down in run_slot)
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
//Parameters: client_fd, slot - slot number (1 based) used for the server messages
//return values: none

static void handle_client(int client_fd, int slot)
{
  if (send_message(client_fd, MSG_HELLO) == -1)
      return;


  char buf[MAX_MESSAGE];
  while (receive_message(client_fd, buf, sizeof(buf)) > 0) {
     strip_newline(buf);

      if (command_type(buf) == CMD_READY)
         play_game(client_fd, slot);    // client sends READY again to play another round
      else
        break;                         // QUIT or anything else ends the session
  } //end while

} //end handle client

//Function name: run_slot
//Description: Each slot (port) gets its own process that just waits on accept() for its one port.
//If a game is already running on this slot, the new client is sent MSG_BUSY and the
//client tries the next port. Otherwise the game is handled in a forked child.
//Parameters: listen_fd - this slot's listening socket, slot - slot number (1 based), port - this slot's port
//return values: none (runs forever)

static void run_slot(int listen_fd, int slot, int port)
{
	pid_t child = 0;                       // 0 means no game running on this slot

	while (1) {
		int client_fd = accept(listen_fd, NULL, NULL);
		if (client_fd < 0) {
			perror("accept");
			continue;
		}

		// waitpid returns 0 if the child is still running, so the slot is busy
		if (child != 0 && waitpid(child, NULL, WNOHANG) == 0) { //If the client is busy utilizing a port and another client is attempting to connect, 
									//MSG busy is shown. Without it the new client attemping to connect is waiting for the
									//child process to finish.
			send_message(client_fd, MSG_BUSY);   		//client will try the next port num
			close_connection(client_fd);
			continue;
		}
		child = 0;                         // previous game (if any) is finished and cleaned up

		printf("Client connected on port %d (slot %d)\n", port, slot);

		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			close_connection(client_fd);
			continue;
		}

		if (pid == 0) {
			srand((unsigned)getpid());
			handle_client(client_fd, slot);
			close_connection(client_fd);
			printf("(Slot %d) Client session ended.\n", slot);
			exit(0);
		}

		child = pid;
		close_connection(client_fd);
	}
} // end run_slot

//function name: main
//description: This function, main, starts the whole server and promotes a persistent connection
//Parameters: argc, argv
//return values: 0 on normal exit, 1 on a startup issue



int main(int argc, char *argv[])
{
	int port = DEFAULT_PORT;

        signal(SIGPIPE, SIG_IGN);

	if (argc > 2) {
	    fprintf(stderr, "Usage: %s [port]\n", argv[0]);
	    return 1;
        }

	//the port needs MAX_PLAYERS consecutive ports (one per slot), so the base port
	//is capped at 65535 - (MAX_PLAYERS - 1), which is 65526 for 10 players.
	//when the port is iterating, and it goes over it can detect that it will go over.
	if (argc == 2) {
	    port = atoi(argv[1]);
	    if (port < 1024 || port > 65535 - (MAX_PLAYERS - 1)) {
	        fprintf(stderr, "Port must be a number from 1024 to %d.\n", 65535 - (MAX_PLAYERS - 1));
		return 1;
	    }
	}

	if (load_words(WORD_FILE) == -1)
	    return 1;
        printf("Loaded %d words.\n", bank_count);

        int listen_fd[MAX_PLAYERS];

        for (int i = 0; i < MAX_PLAYERS; i++) {
	       listen_fd[i] = make_listener(port + i);
               if (listen_fd[i] < 0)
     		   return 1;
               printf("Slot %d listening on port %d\n", i + 1, port + i);
         }

	//one process per slot, each one only accepts on its own port
	for (int i = 0; i < MAX_PLAYERS; i++) {
	       pid_t pid = fork();
	       if (pid < 0) {
	           perror("fork");
	           return 1;
	     }
	     if (pid == 0)
	         run_slot(listen_fd[i], i + 1, port + i);   // never returns
	 }

	//the parent doesn't accept anything, it just waits on the slot processes
	 while (wait(NULL) > 0)
	     ;
	 return 0;
} //end main



























































