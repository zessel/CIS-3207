#include "simpleServer.h"

char **dictionary;
size_t arraySize; 

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
    int connectionPort;
    char *dictionary_path;
    switch (argc)
    {
    case 3: dictionary_path = argv[2];
        printf("case 3");
        connectionPort = atoi(argv[1]);        
        break;
    case 2: dictionary_path = DEFAULT_DICTIONARY;
        connectionPort = atoi(argv[1]);
        printf("case 2");
        break;
    case 1: dictionary_path = DEFAULT_DICTIONARY;
        connectionPort = DEFAULT_PORT;
        printf("case 1");
        break;
    default: perror("Too many arguments\n");
        printf("default");
        return -1;
        break;
    }
    
    dictionary = readInDict(dictionary_path);

    //We can't use ports below 1024 and ports above 65535 don't exist.
    if(connectionPort < 1024 || connectionPort > 65535)
    {
        printf("Port number is either too low(below 1024), or too high(above 65535).\n");
        return -1;
    }

    //sockaddr_in holds information about the user connection. 
    //We don't need it, but it needs to be passed into accept().
    struct sockaddr_in client;
    int clientLen = sizeof(client);
    int connectionSocket, clientSocket, bytesReturned;
    char recvBuffer[BUF_LEN];
    recvBuffer[0] = '\0';
    connectionPort = atoi(argv[1]);
    
    //Does all the hard work for us.
    connectionSocket = open_listenfd(connectionPort);
    if(connectionSocket == -1)
    {
        printf("Could not connect to %s, maybe try another port number?\n", argv[1]);
        return -1;
    }

    //accept() waits until a user connects to the server, writing information about that server
    //into the sockaddr_in client.
    //If the connection is successful, we obtain A SECOND socket descriptor. 
    //There are two socket descriptors being used now:
    //One by the server to listen for incoming connections.
    //The second that was just created that will be used to communicate with 
    //the connected user.
    if((clientSocket = accept(connectionSocket, (struct sockaddr*)&client, &clientLen)) == -1)
    {
        printf("Error connecting to client.\n");
        return -1;
    }
    addToClientQueue(clientSocket);

    printf("Connection success!\n");
    char* clientMessage = "Hello! I hope you can see this.\n";
    char* msgRequest = "Send me some text and I'll respond with something interesting!\nSend the escape key to close the connection.\n";
    char* msgResponse = "I actually don't have anything interesting to say...but I know you sent ";
    char* msgPrompt = ">>>";
    char* msgError = "Message not recieved\n";
    char* msgClose = "Goodbye!\n";

    //send()...sends a message.
    //We specify the socket we want to send, the message and it's length, the 
    //last parameter are flags.
    send(clientSocket, clientMessage, strlen(clientMessage), 0);
    send(clientSocket, msgRequest, strlen(msgRequest), 0);

    //Begin sending and receiving messages.
    while(1)
    {
        send(clientSocket, msgPrompt, strlen(msgPrompt), 0);
        
        //recv() will store the message from the user in the buffer, returning
        //how many bytes we received.
        bytesReturned = recv(clientSocket, recvBuffer, BUF_LEN, 0);
        
        //Check if we got a message, send a message back or quit if the
        //user specified it.
        if(bytesReturned == -1)
        {
            send(clientSocket, msgError, strlen(msgError), 0);
        }
        //'27' is the escape key.
        else if(recvBuffer[0] == 27)
        {
            send(clientSocket, msgClose, strlen(msgClose), 0);
            close(clientSocket);
            break;
        }
        else
        {
            send(clientSocket, msgResponse, strlen(msgResponse), 0);
            send(clientSocket, recvBuffer, bytesReturned, 0);
        }
    }
    return 0;
}

void addToClientQueue(int clientsocket)
{
    getlock;
    {
        clientqueue[nextempty] = clientsocket;
        clients++
        nextempty++;
    }
    releaselock;
}

int retrieveFromClientQueue()
{
    int clientsocket;
    getlock;
    {
        clientsocket = clientqueue[nextclient];
        clients--;
        nextclient++;
    }
    releaselock;
    return clientsocket; 
}

void prepareForLogging(char* result)
{
    getlock;
    {
        logqueue[nextentry] = entry;
        tolog++;
        nextentry++;
    }
    releaselock;
}

char* takeForLogging()
{
    char *entry;
    getlock;
    {
        entry = logqueue[nextforlog];
        tolog--;
        nextforlog++;
    }
    releaselock;
}

/*  Slightly wasteful but I just made the entry big enough to hold
    OK or MISSPELLED
*/
char* resultConcat(char *word, int found, int bytesReturned)
{  
    char *entry = malloc(bytesReturned + 12);
    strcpy(entry, word);
    if (found == 1)
    {
        strcat(entry, " OK");
    }
    else
    {
        strcat(entry, " MISSPELLED");
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
    while(gothrough(fp, &newLine));
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
    printf("\n\n The array size is %zd\n\n", arraySize);

    char **dictionary = (char**) (malloc(arraySize*sizeof(char*)));
    for (size_t i = 0; i < arraySize; i++)
    {
        length = 0;
        getline(&dictionary[i], &length, dictfp);
    }
    return dictionary;        
}

//  Originally I was going to get the word count using the code from wc.c
//  but in the case of this dicationarys format it's the same as counting lines
//  https://www.gnu.org/software/cflow/manual/html_node/Source-of-wc-command.html 


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
int processRequest ()
{
    int clientSocket, bytesReturned;
    char recvBuffer[BUF_LEN];
    char *response;
    while(1)
    { 
        clientSocket = retrieveFromClientQueue();

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
            prepareForLogging(response);
            close(clientSocket);
        }
    }
}

// This will be the thread for logging 
int processLog ()
{
    char *entry;
    FILE *logfp = fopen("log.txt", "w");
    if (!logfp)
    {
        fprintf(stderr, "Cannot open log.txt");
        exit(-1);
    }
    while(1)
    {
        entry = takeForLogging();
        fprintf(logfp, "%s\n", entry);
    }
    perror("Log thread terminated while loop");
    exit(-1);
}
