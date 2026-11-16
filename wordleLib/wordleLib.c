#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "wordleLib.h"


int send_message(int socket_fd, const char *message){
	int bytes_sent;

	bytes_sent = send(socket_fd, message, strlen(message), 0); // create the actual send and store it in bytes_sent 

	if (bytes_sent < 0) // if swend returns something less than 0 aka neg number then we shoot an error
	{
		perror("send");
		return -1;
	} // end perror iff
	return bytes_sent; 
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
