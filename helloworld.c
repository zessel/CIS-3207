#include <stdio.h>

void main ()
{
    printf("Hello World\n");
    int a = 5;
    int d = 7;
    int *b = &a;
    int *c = b;

    a++;
    b = &d;

    printf("%d\n\n", *c);
}