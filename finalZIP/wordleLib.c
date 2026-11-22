/************************************************************/
/* Author: Hunter Phillips                                  */
/* Major: Computer Science                                  */
/* Creation Date: November 3rd, 2026                        */
/* Due Date: November 23rd, 2026                            */
/* Course: CSC328 010                                       */
/* Professor Name: Dr. Walther                              */
/* Assignment: Wordle Group Project                         */
/* Filename: wordleLib.c                                    */
/* Purpose: This file contains the libraries shared functions,
these functions are used by both the server and client.      */                         
/************************************************************/



#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "wordleLib.h"

#include <ctype.h>

/*************************************************************************/
/*                                                                       */
/* Function name: send_message 		                                                */
/* Description: Sends a message that is terminated with the \n operator
through the conneced socket. Loops through until the entire message 
is sent 														         */
/* Parameters: int socket_fd - socket used to send message
			   char *message - the message                                              			 */
/* Return Value: int - total bytes sent or -1 if an error occurs													 */
/*                                                                       */
/*************************************************************************/

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

/*************************************************************************/
/*                                                                       */
/* Function name: receive_message	                                     */
/* Description: Receives messages one byte at a time until a \n character
is read or the buffer is full.														 */
/* Parameters: int socket_fd - socket used to send message
			   char *buffer - the array where the message is stored
			   int buffer_size - max size of buffer.                                                 			 */
/* Return Value: int - number of bytes received or <= 0 if the connection 
closes or there is an error.									       	 */
/*************************************************************************/

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


/*************************************************************************/
/*                                                                       */
/* Function name: close_connection 		                                 */
/* Description: closes the connection from either side													 */
/* Parameters: int socket_fd - the socket the connection is on. 		 */
/* Return Value: N/A													 */
/*                                                                       */
/*************************************************************************/


	void close_connection(int socket_fd){
		close(socket_fd);
	} 


/*************************************************************************/
/*                                                                       */
/* Function name: normalize_word		                                 */
/* Description: changes all letters of the received word to lowercase.   */
/* Parameters: char *word - the word to be normalize_word				 */
/* Return Value: N/A													 */
/*                                                                       */
/*************************************************************************/


	void normalize_word(char *word){
		int i;

		for (i = 0; word[i] != '\0'; i++){ // nromalize each letter to lower case 
			word[i] = tolower((unsigned char)word[i]);
		}
	}// end norm word


/*************************************************************************/
/*                                                                       */
/* Function name: command_type		                                                */
/* Description: deciphers which one of our decided protocol commands was 
received 																 */
/* Parameters: char *message - receieved protocol message                                            			 */
/* Return Value: int - what type of command it was  					 */
/*                                                                       */
/*************************************************************************/


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



/*************************************************************************/
/*                                                                       */
/* Function name: validate_word	                                         */
/* Description: Checks whether a word consists of 5 valid letters		 */
/* Parameters: const char *word - the word                                             			 */
/* Return Value: int - 1 if valid 0 if not													 */
/*                                                                       */
/*************************************************************************/	


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



/*************************************************************************/
/*                                                                       */
/* Function name:strip_newline		                                                */
/* Description: Removes the first newline chracter from a string		*/
/* Parameters: char *s - the string to modify							 */
/* Return Value: N/A													 */
/*                                                                       */
/*************************************************************************/	


    void strip_newline(char *s)
    {
	s[strcspn(s, "\r\n")] = '\0';
    }
