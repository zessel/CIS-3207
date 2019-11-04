#include <stdio.h>
#include "simpleServer.h"
#include <time.h>

#define WORKER_MAX 20
#define WORKER_LOOPS 5
size_t arraySize;
char **dictionary;
int portnumber = 3207;
char *localhost = "127.0.0.1";

int goThroughLine (FILE *fp, size_t* newLine)
{
    int c;

    if (feof (fp))
        return 0;
      
    while ((c = getc (fp)) != EOF)
    {
        if ((c) == '\n')
        {
            (*newLine)++;
            break;
        }
    }
    return c != EOF;
}
size_t getLineCount (FILE *fp)
{
    size_t newLine = 0;
    while(goThroughLine(fp, &newLine));
    return newLine;
}
char** readInDict (char *dictionaryPath)
{  
    char *input; 
    size_t length;
    FILE *dictp = fopen(dictionaryPath, "r");
    if (!dictp)
    {
        fprintf(stderr, "Cannot open file %s", dictionaryPath);
        exit(-1);
    }
    arraySize = getLineCount(dictp);

    fseek(dictp,0,SEEK_SET);

    char **dictionary = (char**) (malloc(arraySize*sizeof(char*)));
    for (size_t i = 0; i < arraySize; i++)
    {
        length = 0;
        getline(&dictionary[i], &length, dictp);
    }
    return dictionary;        
}

void* annoyServer(void *argument)
{
    int bytesReturned;
    char junktext[BUF_LEN];
    printf("WORKERMADE\n");
    for (int i= 0; i < WORKER_LOOPS; i++)
    {
    int hostSocket = open_clientfd(localhost, portnumber);
    if(hostSocket == -1 || hostSocket == -2){
        printf("Could not connect to %d, maybe try another port number?\n", portnumber);
        exit (-1);
    }
    char *query = dictionary[(rand() % arraySize)];
    bytesReturned = recv(hostSocket, junktext, BUF_LEN, 0);
    if(bytesReturned == -1){
        printf("Error reading from server\n");
    }
    else
    {
        printf("sending\n");
        send(hostSocket, query, strlen(query), 0);
    }
    //printf("%s", junktext);
    close(hostSocket);
    }
} 

int main(int argc, char** argv)
{
    dictionary = readInDict("clientdict.txt");
    //if (argc == 2)
    //    portnumber = atoi(argv[1]);

    pthread_t workers[WORKER_MAX];

    for(int i = 0; i < WORKER_MAX; i++)
    {
        if(pthread_create(&workers[i], NULL, annoyServer, NULL) != 0)
        {
            fprintf(stderr, "Error creating worker thread %d\n", i);
        }
    }
    for(int i = 0; i < WORKER_MAX; i++)
    {
        pthread_join(workers[i], NULL);
    }
}