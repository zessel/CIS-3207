#include "simpleServer.h"
#include <ctype.h>

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

int main (int argc,char **argv)
{
    int connectionPort;
    char *dictionary_path;
    switch (argc)
    {
    case 3: 
        dictionary_path = argv[2];
        printf("case 3");
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
        
        printf("case 2");
        break;
    case 1: 
        dictionary_path = DEFAULT_DICTIONARY;
        connectionPort = DEFAULT_PORT;
        printf("case 1");
        break;
    default: perror("Too many arguments\n");
        printf("default");
        return -1;
    }

    printf("\nthe port %d and dict %s\n\n", connectionPort, dictionary_path);
}