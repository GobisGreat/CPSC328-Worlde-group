#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>


#define DEFAULT_PORT 7000

int main(int argc, char *argv[]) {
	int port = DEFAULT_PORT;

	if (argc == 2) {
		port = atoi(argv[1]) // if a user inputs a port it chnges from 7000 to whatever port they input
	} // end if

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);  // create socket over ipv4 nd tcp
	
	if (server_fd < 0) {
		perror("Socket Error");
		
		return 1;

	} // end if 

	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr)); // clear the struct so theres nothing in the filed we aren't setting

	server_addr.sin_family = AF_INET;   
	
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr *)&server_addr, // need tp do this bcuz it's IPv4 specific so needs it be generic 
		sizeof(server_addr)) < 0) {
		perror("Binding error");
	close(server_fd);
	return 1;
	} // end of if

	if (listen(server_fd, 5) < 0) {
		perror("listen");
		close(server_fd);
		return 1;

	} // end listen if 

	printf("Server listening on port %d...\n", port);


	int client_fd = accept(server_fd, NULL, NULL); // this is the listneing  socket
	if (client_fd < 0) {
		perror("accept");
		close(server_fd);
		return 1;
	} // end accept if 

	printf("Client connected. \n");



	const char *hello = "HELLO\n"; // using \n based on our protocol
	send(client_fd, hello, strlen(hello), 0);


	close(client_fd); // closes the connection with a client

	close(server_fd); // closes listen socket


	return 0;
	

} // end main
