#include "simpleServer.h"

int main(int argc, char** argv){

    struct sockaddr_in client;
    int clientLen = sizeof(client);
    int connectionPort = atoi(argv[1]);
    int connectionSocket, clientSocket, bytesReturned;
    char recvBuffer[BUF_LEN];
    recvBuffer[0] = '\0';
    connectionPort = atoi(argv[1]);
    
    if(connectionPort < 1024 || connectionPort > 65535){
        printf("Port number is either too low(below 1024), or too high(above 65535).\n");
    return -1;
    }
    connectionSocket = open_listenfd(connectionPort);
    if(connectionSocket == -1){
        printf("Could not connect to %s, maybe try another port number?\n", argv[1]);
        return -1;
    }
    if((clientSocket = accept(connectionSocket, (struct sockaddr*)&client, &clientLen)) == -1){
        printf("Error connecting to client.\n");
        return -1;
    }
    printf("Connected\n");
    char* clientMessage = "All I do is echo.\n";
    char* msgPrompt = ">>>";
    char* msgError = "I didn't get your message. ):\n";
    char* msgClose = "socket closed\n";
    send(clientSocket, clientMessage, strlen(clientMessage), 0);

    //Begin sending and receiving messages.
    while(1){
        send(clientSocket, msgPrompt, strlen(msgPrompt), 0);
        
        bytesReturned = recv(clientSocket, recvBuffer, BUF_LEN, 0);
        
        if(bytesReturned == -1){
            send(clientSocket, msgError, strlen(msgError), 0);
        }
        
        //'27' is the escape key.
        else if(recvBuffer[0] == 27){
            send(clientSocket, msgClose, strlen(msgClose), 0);
            close(clientSocket);
            break;
        }
        else{
            send(clientSocket, recvBuffer, bytesReturned, 0);
        }
    }
return 0;
}