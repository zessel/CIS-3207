#ifndef _SIMPLE_SERVER_H
#define _SIMPLE_SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define BUF_LEN 512
#define DEFAULT_DICTIONARY "words.txt"
#define DEFAULT_PORT 3207
#define LOG_ENTRY_MAX 30
#define MY_MSG_ERROR "Message not recieved\n"
#define MY_MSG_CLOSE "Terminating connection....\n"
int open_listenfd(int);
#endif