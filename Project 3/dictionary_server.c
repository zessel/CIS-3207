#include "simpleServer.h"
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
    
    //We can't use ports below 1024 and ports above 65535 don't exist.
    if(connectionPort < 1024 || connectionPort > 65535)
    {
        printf("Port number is either too low(below 1024), or too high(above 65535).\n");
        return -1;
    }
    
    dictp = fopen(dictionary_path, "r");
    if (dictp == NULL)
    {
        perror("Could not find dictionary\n");
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
    printf("Connection success!\n");
    char* clientMessage = "Hello! I hope you can see this.\n";
    char* msgRequest = "Send me some text and I'll respond with something interesting!\nSend the escape key to close the connection.\n";
    char* msgResponse = "I actually don't have anything interesting to say...but I know you sent ";
    char* msgPrompt = ">>>";
    char* msgError = "I didn't get your message. ):\n";
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

// This function will read the dictionary file into some other structure for the thread to use
// return type will probably change to return however I store the file
// using
char** readInDict (char *dictionaryPath)
{   
    size_t length;
    int arraySize; 
    FILE *dictp = fopen(dictionaryPath, "r");
    if (!dictp)
    {
        fprintf(stderr, "Cannot open file %s", dictionaryPath);
        exit(-1);
    }
    arraySize = getLineCount(dictp);
    char **dictionary = (char**) (malloc(arraySize*sizeof(char*)));
    for (int i = 0; i < arraySize; i++)
    {
        length = 0;
        getline(&dictionary[i], &length, dictp);
    }
    return dictionary;        
}

//  Originally I was going to get the word count using the code from wc.c
//  but in the case of this dicationarys format it's the same as counting lines
//  https://www.gnu.org/software/cflow/manual/html_node/Source-of-wc-command.html 
int getwordCount (FILE *fp)
{
    int c;
    int newLine = 0;
  
    if (feof (fp))
        return 0;
      
    while ((c = getc (fp)) != EOF)
    {
        if ((c) == '\n')
        {
            newLine++;
            break;
        }
    }
    return c != EOF;
}

int getLineCount (FILE * fp)



// This will be a thread that is handed a client
// process the spelling request
// might be broken into more pieces 
int processRequest ()
{
}

// This will be the thread for logging 
int processLog ()
{
}
