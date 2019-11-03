#ifndef _SIMPLE_SERVER_H
#define _SIMPLE_SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#define BUF_LEN 512
#define DEFAULT_DICTIONARY "words.txt"
#define DEFAULT_PORT 3207
#define MY_MSG_ERROR "Message not recieved\n"
#define MY_MSG_CLOSE "Terminating connection....\n"
#define MY_PROMPT "\n\x1b[36mPlease enter a word for spellchecking or 'Esc' to exit\n\x1b[32m>>> \x1b[0m"
#define SOCKET_BUFFER 20
#define LOG_BUFFER 20
#define MAX_WORKERS 10
int open_listenfd(int);
#endif