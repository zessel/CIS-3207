#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

extern char ** environ;

void parser ();//int *inputflag, int *outputflag, int * createoutflag, int *pipeflag, int *backgroundflag);
void print_args(int argsnum, char*argument_array[]);
char *read_user_input();
int tokenize(char *input, char* argument_array[]);
int check_args_for(int argsnum, char*argument_array[], char *value);
void quit();
void cd (char* new_directory);
void clr();
void dir (char* directory);
void environs();
void echo(char *arguments[]);
void help(char* arguments[]);
void my_pause();

void main()
{
    char *argument_array[64];
    char *input;
    int input_flag, output_flag, output_create_flag, pipe_flag, background_flag;

    while (1)
    {
        input = read_user_input();
        int argsnum = tokenize(input, argument_array);

        input_flag = check_args_for(argsnum, argument_array, "<");
        output_flag = check_args_for(argsnum, argument_array, ">");
        output_create_flag = check_args_for(argsnum, argument_array, ">>");
        pipe_flag = check_args_for(argsnum, argument_array, "|");
        background_flag = check_args_for(argsnum, argument_array, "&");
        
        if (pipe_flag)
        {

        }
        else
        {
            int built_in = check_shell_function(argsnum, argument_array);
            if (built_in > 0)
            {
                run_shell_function(argsnum, argument_array, built_in);
            }
            else
            {
                externalFunction();
            }
            
        }
        
    }
    my_pause();
    help(Zach);
}

/*  Checks if a function built into the shell is called at arg[0] 
    returns an identified for that function
*/

int shellFunction(int argsnum, char *argument_array[])
{
    int is_function = 0;

    if (strcmp(argument_array[0], "cd") == 0)
        is_function = 1;
    else if (strcmp(argument_array[0], "clr") == 0)
        is_function = 2;
    else if (strcmp(argument_array[0], "dir") == 0)
        is_function = 3;
    else if (strcmp(argument_array[0], "environ") == 0)
        is_function = 4;
    else if (strcmp(argument_array[0], "echo") == 0)
        is_function = 5;
    else if (strcmp(argument_array[0], "help") == 0)
        is_function = 6;
    else if (strcmp(argument_array[0], "pause") == 0)
        is_function = 7;
    else if (strcmp(argument_array[0], "quit") == 0)
        is_function = 8;    

    return is_function;
}

void run_shell_function(int argnum, char *argument_array[], int built_in)
{

}



void parser ()//int *inputflag, int *outputflag, int * createoutflag, int *pipeflag, int *backgroundflag)
{
    char *input = read_user_input();
    char *argument_array[64];
    int argsnum = tokenize(input, argument_array);
    //void mark_flags(int argsnum, char*argument_array[], &input_flag, &output_flag, &pipe_flag, );
    print_args(argsnum, argument_array);
    //set flags true/false
}

/*  Print out all arguments.  Only testing */
void print_args(int argsnum, char*argument_array[])
{
    printf("\nThere are %d arguments", argsnum);
    for(int i = 0; i < argsnum; i++)
    {
        printf("\nArgument %d: %s", i, argument_array[i]);
    }
}

/* This will return the users input stream as a string.  If input and length are NULL and 0 getline handles the buffer allocation, user handles the free*/
char *read_user_input()
{
    char *input = NULL;
    size_t length = 0;
    getline(&input, &length, stdin);    
    return input;
}

/* This will give us the number of arguments the user gave us as well as separate the arguments into individual strings */ 
int tokenize(char *input, char* argument_array[])
{
    int words = 0;
    char *token = strtok(input, " ");
    while (token != NULL)
    {
        argument_array[words] = token;
        token = strtok(NULL, " ");
        words++;
    }
    return words;
}

/*  A simple visitor type method to check for string matches in arguments
    used to find redirection modifiers "<" ">" ">>" "|" arguments */
int check_args_for(int argsnum, char*argument_array[], char *value)
{
    int hit_value = 0;
    for (int i = 0; i < argsnum; i++)
    {
        if (strcmp(argument_array[i], value) == 0)
            hit_value = 1;
    }
    return hit_value;
}

/* Quits shell*/
void quit()
{
    exit(0);
}

/* calls chdir to change the directory to a given path */
// TODO Add proper error printing if time permits
void cd (char* new_directory)
{
    int error;
    if (new_directory == NULL)
    {
        char cwd[PATH_MAX];
        printf("%s", getcwd(cwd, sizeof(cwd)));
    }
    else 
        error = chdir(new_directory);
    if (error = -1)
    { 
        perror("cd");
    }
}

/* clears screen */
void clr()
{
    printf("\033[H\033[2J");
}

/*  print contents of a directory 

    Code taken almost directly from the manpage for 'opendir'

*/
void dir (char* directory)
{
    DIR *direct;
    struct dirent *dptr;
    if ((direct = opendir (directory)) == NULL) 
    {
        perror ("dir");
    }
    else
    {
        while ((dptr = readdir (direct)) != NULL) 
        {        
            printf("%s\n", dptr->d_name);
        }
    }
}

/* prints environment strings */
void environs()
{
    for(int i = 0; environ[i] != NULL; i++)
    {
        printf("%s\n", environ[i]);
    }
}

/* prints all arguments */
void echo(char *arguments[])
{
    for(int i = 0; arguments[i] != NULL; i++)
    {
        printf("%s\n", arguments[i]);
    }
}

/*  opens a readme file and pauses when the terminal is full (simulates "more")

    This isn't quite working how I would like it too, I might need to make a custom
    pause function that takes any key input.  But I can't figure out how to
    take the users return and not have it output in the terminal like the actual
    more or less commands in linux.

    Also it occasionally stops printing midword when it doesn't appear the be at the end of
    the terminal screen.  This only happens after the initial pause which has worked great so far.
    Possible that might nested loops logic is a little off 
 */
void help(char* arguments[])
{
    clr();
    FILE *filepointer;
    if ((filepointer = fopen("readme.md", "r")) == NULL)
        perror("help");        
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    char nextchar = fgetc(filepointer);
    int row = 1;
    int col = 1;
    while(nextchar != EOF)
    {
        printf("%c", nextchar);
        col++;
        if (col > w.ws_col)
        {
            col = 1;
            row++;
        }
        if (nextchar == '\n')
        {
            row++;
        }
        nextchar = fgetc(filepointer);
        if (row == w.ws_row - 1)
        {
            my_pause();
            clr();
            row = 1;
        }
    }
    fclose(filepointer);
    printf("\n");
}

/* clears stdin buffer, waits until it finds a return in the stdin buffer, then clears the buffer */
void my_pause()
{
    fflush(stdin);
    printf("\nPress enter to continue");
    char need_enter = getchar();
    while (need_enter != '\n')
        need_enter = getchar();
    fflush (stdin);
}
