#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "wordleLib.h"

#include <ctype.h>


int send_message(int socket_fd, const char *message){
	
	int total = 0;
	int message_length = strlen(message);


	while (total < message_length) // i added this loop to this version bcuz tcp technically doesnt auto send everything in one message
	{

	int bytes_sent;

	bytes_sent = send(socket_fd, message + total, message_length - total, 0); // create the actual send and store it in bytes_sent 

	if (bytes_sent <= 0) // if swend returns something less than 0 aka neg number then we shoot an error
	{
		perror("send");
		return -1;
	} // end perror iff
	

	total += bytes_sent;


} // end while	

	return total; 
} // end send message

int receive_message(int socket_fd, char *buffer, int buffer_size){
	int total = 0;
	char current;

	while (total < buffer_size - 1) { // loop thru till the buffer is full -1 buffer size
	
	int bytes = recv(socket_fd, &current, 1, 0); // receives one byte and stores it in current 

	if (bytes <= 0) //other side closed connection
	{
		return bytes; 
	} 


	buffer[total] = current; // store current into index rep'd by total
	total++;

	if (current == '\n'){ // message over 
		break;
	} // end if

	} // end loop 	

	buffer[total] = '\0'; // end of string 

	return total;

	} // end recieve message 


	void close_connection(int socket_fd){
		close(socket_fd);
	} 


	void normalize_word(char *word){
		int i;

		for (i = 0; word[i] != '\0'; i++){ // nromalize each letter to lower case 
			word[i] = tolower((unsigned char)word[i]);
		}
	}// end norm word


	int command_type(const char *message){
		if (strncmp(message, "HELLO", 5) == 0) // compare message to hello,ready,quit if its one of them return that cmd type and if not it's cmd unknown
		{
			return CMD_HELLO;
		} // nd hello
		if (strncmp(message, "READY", 5) == 0){
			return CMD_READY;

	} // end ready
		if (strncmp(message, "QUIT", 4) == 0){
			return CMD_QUIT;

		} // end quit

		return CMD_UNKNOWN;
	} // end command type


	int validate_word(const char *word){ // important distincition; this does not check it with the word bank, this only checks for 5 letter lenght and all 5 r letters
		int i;

		if (strlen(word) != 5){
			return 0;
		} // end if not 5

		for (i = 0; i < 5; i++){
			if (!isalpha((unsigned char)word[i])) // is alpha is is it a letter? so this is just checking letter by letter ensuring it's a letter
			{
				return 0;
			} // end if
		} // end for
		return 1;
	} // end validate word


	int word_in_bank(const char *word, char word_bank[][6], int word_count){
		// im gonna use a binary search for this cuz the list is like 3 thousand or so long
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
    void strip_newline(char *s)
    {
	s[strcspn(s, "\r\n")] = '\0';
    }
