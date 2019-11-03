#include "simpleServer.h"

char ** dictionary;
size_t arraySize; 

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
    printf("\n\n The array size is %zd\n\n", arraySize);

    char **dictionary = (char**) (malloc(arraySize*sizeof(char*)));
    for (size_t i = 0; i < arraySize; i++)
    {
        length = 0;
        getline(&dictionary[i], &length, dictp);
    }
    return dictionary;        
}

void main ()
{
    size_t i = 0;
    dictionary = readInDict("words.txt");
    while (dictionary[i] != NULL)
    {
        printf("%zd %s", i, dictionary[i]);
        i++;
    }
}

