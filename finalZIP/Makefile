# File: Makefile
# Author: Gabe Segaline Major: IT, Ryan Doan Major: CS, Hunter Phillips Major: CS
# Course: CPSC 328
# Creation date: 11/23/26
# Due date: 11/23/26
# Professor: Walther
# Description: Builds the Wordle client and server.
#              Compiles wordleLib.c into a static
#              library (libwordle.a) and links it
#              into both executables.
#
# Targets:
#   make         - build server and client
#   make server  - build only the server
#   make client  - build only the client
#   make clean   - remove object files, library, and
#                  executables



CC     = gcc
CFLAGS = -Wall -Wextra

all: server client

# compile the shared library source into an object file
wordleLib.o: wordleLib.c
	$(CC) $(CFLAGS) -c wordleLib.c

# create the static library from the object
libwordle.a: wordleLib.o
	ar rc libwordle.a wordleLib.o

# compile server
server: server.c libwordle.a
	$(CC) $(CFLAGS) server.c libwordle.a -o server

# compile client
client: client.c libwordle.a
	$(CC) $(CFLAGS) client.c libwordle.a -o client

clean:
	rm -f *.o *.a server client

.PHONY: all clean
