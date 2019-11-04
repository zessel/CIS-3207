#include "simpleServer.h"

/*  This hold all the condition variable stuff
    This should probably be split into two  structs
    one for each condition 
*/
typedef struct{
    int socketQueue[SOCKET_BUFFER];
    char *logQueue[LOG_BUFFER];
    int clients, nextEmptyClient, nextClient;
    int logs, nextEmptyLog, nextLog;
    pthread_mutex_t sockmutex;
    pthread_mutex_t logmutex;
    pthread_cond_t canAddClient, canRemoveClient;
    pthread_cond_t canAddLog, canRemoveLog;
} buf;

//  Should change these from globals
char **dictionary;
size_t arraySize; 

void initializeStruct(buf *spellBuffer);
void addToClientQueue(int clientsocket, buf *spellBuffer);
int retrieveFromClientQueue(buf *spellBuffer);
void prepareForLogging(char* result, buf *spellBuffer);
char* takeForLogging(buf *spellBuffer);
char* resultConcat(char *word, int found, int bytesReturned);
int goThroughLine (FILE *fp, size_t* newLine);
size_t getLineCount (FILE *fp);
char** readInDict (char *dictionaryPath);
char* checkword(char query[BUF_LEN], int bytesReturned);
void* processRequest (void *args);
void* processLog (void *args);



/*  Is used to determine if the second command line argument is a port
    number or a text file
*/
int isPort(char *argv)
{
    int i = 0;
    int nonNumbflag = 1;
    while (argv[i] != '\0')
    {        
        if (isdigit(argv[i]) == 0)
            nonNumbflag = 0;
        i++;
    }
    return nonNumbflag;
}

/*  Creates the log.txt that's appended to later
*/
void createLogfile()
{
    FILE *logfp = fopen("log.txt", "w");
    if (!logfp)
    {
        fprintf(stderr, "Cannot open log.txt");
        exit(-1);
    }
    fclose(logfp);
}

/*  Creates all the worker threads
    Sends one worker to be the logging thread and the
    rest to be client processing threads.

    Was getting segfaults with this so it's unused
*/
void createWorkers(buf *spellBuffer)
{
    pthread_t workers[MAX_WORKERS];
    if(pthread_create(&workers[0], NULL, processLog, &spellBuffer) != 0)
    {
        perror("Error creating logging thread %d\n");
        exit(-1);
    }
    for(int i = 1; i < MAX_WORKERS; i++)
    {
        if(pthread_create(&workers[i], NULL, processRequest, &spellBuffer) != 0)
        {
        fprintf(stderr, "Error creating worker thread %d\n", i);
        exit(-1);
        }
    }
}

/*  The pthread conditional variables need to
    be initialized so that happens here
    */
void initializeStruct(buf *spellBuffer)
{
    pthread_mutex_init(&spellBuffer->sockmutex, NULL);
    pthread_mutex_init(&spellBuffer->logmutex, NULL);
    pthread_cond_init(&spellBuffer->canAddClient, NULL);
    pthread_cond_init(&spellBuffer->canAddLog, NULL);
    pthread_cond_init(&spellBuffer->canRemoveClient, NULL);
    pthread_cond_init(&spellBuffer->canRemoveLog, NULL);
    spellBuffer->clients = spellBuffer->nextEmptyClient = spellBuffer->nextClient = 0;
    spellBuffer-> logs = spellBuffer->nextEmptyLog = spellBuffer->nextLog = 0;
}

/*  A server that supports spell checking.
    -Creates the log file
    -Creates and intializes the buffer struct
    -Intiliazes a dictionary to compare against
    -Create and send off worker threads
    After that the thread just listens for incoming connections
    if one is established it puts that socket (file descriptor)
    into a queue for the worker threads to deals with

    Should loop indefinitely.
*/
int main(int argc, char** argv)
{
    createLogfile();

    buf spellBuffer;
    initializeStruct(&spellBuffer);

    // Switch to determine what the user wants for the port and dict
    int connectionPort;
    char *dictionary_path;
    switch (argc)
    {
        case 3: 
            dictionary_path = argv[2];
            connectionPort = atoi(argv[1]);        
            break;
        case 2: 
            if (isPort(argv[1]))
            {
                connectionPort = atoi(argv[1]);
                dictionary_path = DEFAULT_DICTIONARY;
            }
            else
            {
                dictionary_path = argv[1];
                connectionPort = DEFAULT_PORT;
            }       
            break;
        case 1: 
            dictionary_path = DEFAULT_DICTIONARY;
            connectionPort = DEFAULT_PORT;
            break;
        default: 
            perror("Too many arguments\n");
            exit (-1);
    }
    
    // After dict is read in does a quick check to make sure it will work 
    dictionary = readInDict(dictionary_path);
    for (int z = 0; z< strlen(dictionary[0]); z++)
    {
        if (dictionary[0][z] == '\r')
        {
            perror("You a using a dictionary with CRLF \"\\r\\n\", \nplease restart with a LF \"\\n\" only file or default dictionary");
            exit(-1);
        }
    }

    pthread_t workers[MAX_WORKERS];
    if(pthread_create(&workers[0], NULL, processLog, &spellBuffer) != 0)
    {
        perror("Error creating logging thread %d\n");
        exit(-1);
    }
    for(int i = 1; i < MAX_WORKERS; i++)
    {
        if(pthread_create(&workers[i], NULL, processRequest, &spellBuffer) != 0)
        {
        fprintf(stderr, "Error creating worker thread %d\n", i);
        exit(-1);
        }
    }

    //We can't use ports below 1024 and ports above 65535 don't exist.
    if(connectionPort < 1024 || connectionPort > 65535)
    {
        printf("Port number is either too low(below 1024), or too high(above 65535).\n");
        return -1;
    }

    //sockaddr_in holds information about the user connection. 
    struct sockaddr_in client;
    int clientLen = sizeof(client);
    int connectionSocket, clientSocket, bytesReturned;
    char recvBuffer[BUF_LEN];
    recvBuffer[0] = '\0';
    
    // open_listenfd from CS:APP by Bryant
    connectionSocket = open_listenfd(connectionPort);
    if(connectionSocket == -1)
    {
        printf("Could not connect to %s, maybe try another port number?\n", argv[1]);
        return -1;
    }

    // Loop to continue adding new connections
    while(1)
    {
        if((clientSocket = accept(connectionSocket, (struct sockaddr*)&client, &clientLen)) == -1)
        {
            printf("Error connecting to client.\n");
            return -1;
        }
        addToClientQueue(clientSocket, &spellBuffer);
    }
    return 0;
}

/*  Adds to the next empty slot of the queue holding client sockets
    Only called by main
    Producer
*/
void addToClientQueue(int clientsocket, buf *spellBuffer)
{
    pthread_mutex_lock(&spellBuffer->sockmutex);
    while(spellBuffer->clients == SOCKET_BUFFER)
    {  
        pthread_cond_wait(&spellBuffer->canAddClient, &spellBuffer->sockmutex);
    }
        spellBuffer->socketQueue[spellBuffer->nextEmptyClient] = clientsocket;
        spellBuffer->clients++;
        spellBuffer->nextEmptyClient = (spellBuffer->nextEmptyClient + 1) % SOCKET_BUFFER;
    pthread_cond_signal(&spellBuffer->canRemoveClient);
    pthread_mutex_unlock(&spellBuffer->sockmutex);
}

/*  Retrieves the next client from the queue holding client sockets
    Only called by worker threads
    Consumer
*/
int retrieveFromClientQueue(buf *spellBuffer)
{
    int clientsocket;
    pthread_mutex_lock(&spellBuffer->sockmutex);
    while(spellBuffer->clients == 0)
    {  
        pthread_cond_wait(&spellBuffer->canRemoveClient, &spellBuffer->sockmutex);
    }    
        clientsocket = spellBuffer->socketQueue[spellBuffer->nextClient];
        spellBuffer->clients--;
        spellBuffer->nextClient = (spellBuffer->nextClient + 1) % SOCKET_BUFFER;
    
    pthread_cond_signal(&spellBuffer->canAddClient);
    pthread_mutex_unlock(&spellBuffer->sockmutex);
    return clientsocket; 
}

/*  Adds a spell checked entry to be logging into the logging queue
    Only called by worker threads
    Producer
*/
void prepareForLogging(char* result, buf *spellBuffer)
{
    pthread_mutex_lock(&spellBuffer->logmutex);
    while(spellBuffer->logs == LOG_BUFFER)
    {  
        pthread_cond_wait(&spellBuffer->canAddLog, &spellBuffer->logmutex);
    }
        spellBuffer->logQueue[spellBuffer->nextEmptyLog] = result;
        spellBuffer->logs++;
        spellBuffer->nextEmptyLog = (spellBuffer->nextEmptyLog + 1) % LOG_BUFFER;
    pthread_cond_signal(&spellBuffer->canRemoveLog);
    pthread_mutex_unlock(&spellBuffer->logmutex);
}

/*  Retrieves a spell checked entry for logging from queue
    Only called by logger thread
    Consumer
*/
char* takeForLogging(buf *spellBuffer)
{
    char *entry;
    pthread_mutex_lock(&spellBuffer->logmutex);
    while(spellBuffer->logs == 0)
    {  
        pthread_cond_wait(&spellBuffer->canRemoveLog, &spellBuffer->logmutex);
    }
        entry = spellBuffer->logQueue[spellBuffer->nextLog];
        spellBuffer->logs--;
        spellBuffer->nextLog = (spellBuffer->nextLog + 1) % LOG_BUFFER;
    pthread_cond_signal(&spellBuffer->canAddLog);
    pthread_mutex_unlock(&spellBuffer->logmutex);
    return entry;
}

/*  Concatenates the result onto the queried word
    Slightly wasteful malloc but big enough to hold
    OK or MISSPELLED
*/
char* resultConcat(char *word, int found, int bytesReturned)
{  
    char *entry = malloc(bytesReturned + 13); // 1 space + 10 MISSPELLED + 1 \n + 1 \0
    strcpy(entry, word);
    if (found == 1)
    {
        strcat(entry, " OK\n");
    }
    else
    {
        strcat(entry, " MISSPELLED\n");
    }
    return entry;
}

/*  Just goes through the dictionary char by char
    Was originally looking to style it after the wc.c
    code but kind of lost sight of that
*/
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

/*  Counts the lines in the dictionary file
    Was originally looking to style it after the wc.c
    code but kind of lost sight of that
*/
size_t getLineCount (FILE *fp)
{
    size_t newLine = 0;
    while(goThroughLine(fp, &newLine));
    return newLine;
}

/*  This ends of loading the dictionary pointed to by path in arguments or default
    It reads the total lines in the file then mallocs an array of strings for every entry
    Then resets the position in the file and reads back through to fill the array  
*/
char** readInDict (char *dictionaryPath)
{  
    char *input; 
    size_t length;
    FILE *dictfp = fopen(dictionaryPath, "r");
    if (!dictfp)
    {
        fprintf(stderr, "Cannot open file %s", dictionaryPath);
        exit(-1);
    }
    arraySize = getLineCount(dictfp);
    fseek(dictfp,0,SEEK_SET);
    printf("\n\nServer loaded with %zd dictionary entries\n\n", arraySize);

    char **dictionary = (char**) (malloc(arraySize*sizeof(char*)));
    for (size_t i = 0; i < arraySize; i++)
    {
        length = 0;
        getline(&dictionary[i], &length, dictfp);
    }
    return dictionary;        
}

/*  Checks the query against every entry in the dictionary
    I really should have used a more effiecent structure because this
    is wasteful
*/
char* checkword(char query[BUF_LEN], int bytesReturned)
{
    int found = 0;
    for (size_t i = 0; i < arraySize; i++)
    {
        if (strncmp(dictionary[i], query, bytesReturned) == 0){
            found = 1;
            break;
        }
    }
    query[bytesReturned - 1] = '\0';
    return resultConcat(query, found, bytesReturned);
}

/*  Worker thread function
    Infinite loop of
        getting a client file descriptor
        prompting client
        reading clients response
        checking query
        answering client
        sending query for logging
        closing filedescriptor
*/
void* processRequest (void *args)
{
    buf *spellBuffer = (buf*) args;
    int clientSocket, bytesReturned;
    char recvBuffer[BUF_LEN];
    char *response;
    while(1)
    { 
        clientSocket = retrieveFromClientQueue(spellBuffer);
        printf("Servicing socket %d\n", clientSocket);
        send(clientSocket, MY_PROMPT, strlen(MY_PROMPT),0);
        bytesReturned = recv(clientSocket, recvBuffer, BUF_LEN, 0);
        
        //Check if we got a message, send a message back or quit if the
        //user specified it.
        if(bytesReturned == -1)
        {
            send(clientSocket, MY_MSG_ERROR, strlen(MY_MSG_ERROR), 0);
        }
        //'27' is the escape key.
        else if(recvBuffer[0] == 27)
        {
            send(clientSocket, MY_MSG_CLOSE, strlen(MY_MSG_CLOSE), 0);
            close(clientSocket);
            break;
        }
        else
        {        
            response = checkword(recvBuffer, bytesReturned);
            send(clientSocket, response, strlen(response), 0);
            prepareForLogging(response, spellBuffer);
            close(clientSocket);
        }
    }
}

/*  Logger thread function
    Infinite loop of
        Grabbing query from loggin queue
        appending to log
        closing log
*/
void* processLog (void *args)
{
    buf *spellBuffer = (buf*) args;
    char *entry;
    while(1)
    {
        entry = takeForLogging(spellBuffer);
        FILE *logfp = fopen("log.txt", "a");
        if (!logfp)
        {
        fprintf(stderr, "Cannot open log.txt");
        exit(-1);
        }
        fprintf(logfp, "%s", entry);
        fclose(logfp);
    }
    perror("Log thread terminated");
    exit(-1);
}
