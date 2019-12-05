# build a program from 2 files and one shared header
CC = gcc
CFLAGS = -g

go: filesystem.c disk.c
	$(CC) $(CFLAGS) -o go filesystem.c disk.c

filesystem: filesystem.c
	$(CC) $(CFLAGS) -o filesystem filesystem.c