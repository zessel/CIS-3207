#include <pthread.h>
#include <stdio.h>
#define SOCKET_BUFFER 1
#define LOG_BUFFER 1


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


void addToClientQueue(int clientsocket, buf *spellBuffer)
{
    pthread_mutex_lock(&spellBuffer->sockmutex);
    while(spellBuffer->clients == SOCKET_BUFFER)
    {  
        printf("PROCUDER: Buffer is full.  Waiting\n");
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
        printf("CONSUMER: Buffer is empty.  Waiting\n");
        pthread_cond_wait(&spellBuffer->canRemoveClient, &spellBuffer->sockmutex);
    }    
        clientsocket = spellBuffer->socketQueue[spellBuffer->nextClient];
        spellBuffer->clients--;
        spellBuffer->nextClient = (spellBuffer->nextClient + 1) % SOCKET_BUFFER;
    
    pthread_cond_signal(&spellBuffer->canAddClient);
    pthread_mutex_unlock(&spellBuffer->sockmutex);
    return clientsocket; 
}

void * workerStuff (void* nothing)
{
    buf *buffer = (buf *) nothing;
    int i;
    while (1)
    {
        i = retrieveFromClientQueue(buffer);
        printf("CONSUMER: Retrieved %d\n", i);
    }
}

void main ()
{
    buf buffer;
    initializeStruct(&buffer);
    int i = 0;
    pthread_t worker[1];
    if(pthread_create(&worker[0], NULL, workerStuff, &buffer) != 0)
    {
        perror("Error creating worker thread\n");
    }
    while (1)
    {
        i = (i + 1 ) % 4;
        printf("PRODUCER: attempting to add %d to queue\n", i);
        addToClientQueue(i, &buffer);
    }

}