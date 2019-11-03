#include <pthread.h>
#include <stdio.h>

#define MAX_WORKERS 10
void* workerPrint(void *i);


void main (int argc, char** argv)
{
    pthread_t workers[MAX_WORKERS];
    for(int i = 0; i < MAX_WORKERS; i++)
    {
        if(pthread_create(&workers[i], NULL, workerPrint, &i) != 0)
        {
            fprintf(stderr, "Error creating worker thread %d\n", i);
        }
    }
    for(int i = 0; i < MAX_WORKERS; i++)
        pthread_join(workers[i], NULL);
}

void* workerPrint(void *i)
{
    int *n = (int*) i;
    printf("I am worker %d and this is the end for me\n", *n);
}