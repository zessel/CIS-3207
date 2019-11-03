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

char* resultConcat(char *word, int found, int bytesReturned)
{  
    char *entry = malloc(bytesReturned + 13);
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
    query[bytesReturned-1] = '\0';
    return resultConcat(query, found, bytesReturned);
}

#include "simpleServer.h"

int main(int argc, char** argv)
{
    dictionary = readInDict("words.txt");    
    char input[50];
    while(1){
    fgets(input, 50, stdin);
    printf("%s",checkword(input, strlen(input)));
    }
    return 0;
}