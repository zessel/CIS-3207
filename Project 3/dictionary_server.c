#include "simpleServer.h"

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
//An extremely simple server that connects to a given port.
//Once the server is connected to the port, it will listen on that port
//for a user connection.
//A user will connect through telnet, the user also needs to know the port number.
//If the user connects successfully, a socket descriptor will be created.
//The socket descriptor works exactly like a file descriptor, allowing us to read/write on it.
//The server will then use the socket descriptor to communicate with the user, sending and 
//receiving messages.
int main(int argc, char** argv)
{
    createLogfile();

    buf spellBuffer;
    initializeStruct(&spellBuffer);

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
    
    connectionSocket = open_listenfd(connectionPort);
    if(connectionSocket == -1)
    {
        printf("Could not connect to %s, maybe try another port number?\n", argv[1]);
        return -1;
    }

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

/*  Slightly wasteful but I just made the entry big enough to hold
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

// This function will read the dictionary file into some other structure for the thread to use
// return type will probably change to return however I store the file
// using
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

// This will be a thread that is handed a client
// process the spelling request
// might be broken into more pieces 
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

// This will be the thread for logging 
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
