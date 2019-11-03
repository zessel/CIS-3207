# build a program from 2 files and one shared header
CC = gcc
CFLAGS = -g

program: dictionary_server.c open_listenfd.c
	$(CC) $(CFLAGS) -o Project3_Server dictionary_server.c open_listenfd.c -lpthread