#include <stdio.h>

extern char ** environ;

void main(){ 
    environ();
}

void environ(){
  for(int i = 0; environ[i] != NULL; i++)
  {
    printf("%s\n", environ[i]);
  }
}
