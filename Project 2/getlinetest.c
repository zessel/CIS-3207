#include <stdio.h>

void main(int argc, char *argv[])
{
    FILE *stream = fopen(argv[1], "r");
    char *output;
    size_t buf = 0;
    while(getline(&output, &buf, stream) != -1)
        printf("%s", output);
}