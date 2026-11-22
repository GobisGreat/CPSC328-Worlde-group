# Wordle Project

## Authors
- **Gabe Segaline - Project Leader** (Server) 
- Hunter Phillips (Library)
- Ryan Doan (Client)

## Project Overview
This project provides the user with everything they need to run a Wordle game in their terminal. It is a C based implementation using TCP sockets. The Client will connect to the server where it is then assigned a word to guess by the server. The client will have six attempts to guess the word, where it is given feedback on each guess. 

## Files

- `client.c` - Client Program.
- `server.c`- Server Program.
- `wordleLib.c` - Shared library functions for client and server.
- `wordleLib.h` - declerations for shared functions.
- `valid-wordle-words.txt` - word bank used for game.

## Compiling the program
```bash
#compile your shared library into an object file
gcc -c wordleLib.c 

#create the static library from the object
ar rc libwordle.a wordleLib.o

#compile server

gcc server.c libwordle.a -o server

#compile client
gcc client.c libwordle.a -o client
```
## Running the server
To run the server you have two options, you can either run the server with default options. (It is generally a good idea to make sure you know either the server's IP address or hostname before beginning this process)
```
./server
```
Or you can specify a port and file (recommended)
```
./server <portnumber> valid-wordle-words.txt
```
## Running the client 
To run the client you also have the same options as above, although you must always include the IPv4 or Hostname like so:
```
./client <IPv4 or HostName>
```
Alternativley:
```
./client <IPv4 or HostName> <portnumber>
```
If you specify the port number on the server you may find it easier to also specify it on the client side.   

If you are running the server locally you can use 
```
./client 127.0.0.1
```

with or without the port number specified.

## Library Implementation

This project uses a static library

The static library, named wordleLib, holds all common functions between client.c and server.c. These functions include sending and receiving messages, closing socket connections, removing the new line characters from C strings, validating 5 letter words, normalizing words to lower case, and identifying protocol commands.  

We chose static libraries because the client and server can both use the common code. This way the client and server can reuse this library over and over without duplicating the code on both files. It also means this program doesn't require a runtime dependency and is slightly faster than if we used a shared library.

## Protocol 

The client server communication protocol we chose was TCP. All messages are followed by a newline character that signals the end of our application level network messages in C. The 'receive_message()' function reads data until the newline character is read, allowing both the client and server to process these messages and when they are completely received.

Examples of our protocol messages:

- `HELLO\n` - Server tells the client that a connection was accepted.
- `READY\n` - Client tells the server it's ready for a word.
- `QUIT\n` - Client tells the server it is disconnecting.
- `BUSY\n` - Server tells the client that the player slot is taken.
- `INVALID\n` - Server tells the client that the submitted word is invalid.
- `<word>\n` - Client's guess
- `<feedback>\n` - Server sends five feedback characters (G,Y,or X) to the client.


## Known issues

It's hard to identify exaclty how many 5 letter words there are in the english language, with that being the case we found the most plausible list from a GitHub from a user who seemed reputable. However it would be imposssible to verify the entire roughly 16,000 entry array that makes up our list. This means there may be some unreliable words inside our game. 

## Teamwork

We divided the project into three main responsibilites as per the rubric. These being client development, server development, and library development. Each team member worked primarily on their own assigned component however there was slight overlap among all three members. We used Microsoft Teams to communicate and GitHub for version control. We found communication was extemely important to understand what functionality and interpratation of the assignment was the best way to go about development. Throughout development we tested all three phases heavily.
