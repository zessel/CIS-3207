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
#define DICTIONARY_WORD_MAX 25
int open_listenfd(int);
#endif