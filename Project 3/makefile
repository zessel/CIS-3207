# build a program from 2 files and one shared header
CC = gcc
CFLAGS = -g

Project3_Server: dictionary_server.c open_listenfd.c
	$(CC) $(CFLAGS) -o Project3_Server dictionary_server.c open_listenfd.c -lpthread

client: client.c open_clientfd.c
	$(CC) $(CFLAGS) -o client client.c open_clientfd.c -lpthread
