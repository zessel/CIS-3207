#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h> // For PATH_MAX
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>  // For wait()
#include <sys/wait.h>   // For wait()

extern char ** environ;

void external_function(int argsnum, char *argument_array[]);
int check_shell_function(int argsnum, char *argument_array[]);
void run_shell_function(int argnum, char *argument_array[], int built_in);
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
void echo(int argsnum, char *arguments[]);
void help();
void my_pause();

void main()
{
    char *argument_array[64];
    char *input;
    int input_flag, output_flag, output_create_flag, pipe_flag;

    while (1)
    {
        input = read_user_input();
        int argsnum = tokenize(input, argument_array);

        input_flag = check_args_for(argsnum, argument_array, "<");
        output_flag = check_args_for(argsnum, argument_array, ">");
        output_create_flag = check_args_for(argsnum, argument_array, ">>");
        pipe_flag = check_args_for(argsnum, argument_array, "|");
        
        print_args(argsnum, argument_array);

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
                external_function(argsnum, argument_array);
            }
        }
        
    }
    my_pause();
    help();
}


void external_function(int argsnum, char *argument_array[])
{
    int background_flag = check_args_for(argsnum, argument_array, "&");
    int status;

    int pid = fork();
    if (pid == 0)
    {
        execvp(argument_array[0], argument_array);
    }
    else if (!background_flag)
        wait(&status);
}


/*  Checks if a function built into the shell is called at arg[0] 
    returns an identified for that function

    had to change to strncmp because strcmp was only working if the
    command was followed by whitespace.  I imagine there is a difference
    in null char or \n but this should be a fine workaround
*/

int check_shell_function(int argsnum, char *argument_array[])
{
    int is_function = 0;

    if (strncmp(argument_array[0], "cd", 2) == 0)
        is_function = 1;
    else if (strncmp(argument_array[0], "clr", 3) == 0)
        is_function = 2;
    else if (strncmp(argument_array[0], "dir", 3) == 0)
        is_function = 3;
    else if (strncmp(argument_array[0], "environ", 7) == 0)
        is_function = 4;
    else if (strncmp(argument_array[0], "echo", 4) == 0)
        is_function = 5;
    else if (strncmp(argument_array[0], "help", 4) == 0)
        is_function = 6;
    else if (strncmp(argument_array[0], "pause", 5) == 0)
        is_function = 7;
    else if (strncmp(argument_array[0], "quit", 4) == 0)
        is_function = 8;    

    return is_function;
}

void run_shell_function(int argsnum, char *argument_array[], int built_in)
{
    switch(built_in)
    {
        case 1: cd(argument_array[1]);
            break;
        case 2: clr();
            break;
        case 3: dir(argument_array[1]);
            break;
        case 4: environs();
            break;
        case 5: echo(argsnum, argument_array);
            break;
        case 6: help();
            break;
        case 7: my_pause();
            break;
        case 8: quit();
            printf("This shouldn't be reached");
            break;
    }
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
    char *token = strtok(input, " \t\n");
    while (token != NULL)
    {
        argument_array[words] = token;
        token = strtok(NULL, " \t\n");
        words++;
    }
    argument_array[words] = NULL;
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
    {
        error = chdir(new_directory);
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

    modified to use cwd on NULL to work around seg faults

*/
void dir (char* directory)
{
    if (directory == NULL)
    {
        char cwd[PATH_MAX];
        directory = getcwd(cwd, sizeof(cwd));
    }
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

/*  prints all arguments up to an output redirect follow by a single return */
void echo(int argsnum, char *arguments[])
{
    int still_comment = 1;
    int i = 1;
    while(still_comment)
    {
        if ((i >= argsnum) 
            || (strcmp(arguments[i], ">") == 0) 
            || (strcmp(arguments[i], ">>") == 0))
        {
            still_comment = 0;
        }
        else
        {  
            printf("%s ", arguments[i]);
            i++;
        }
    }
    printf("\n");
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
void help()
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
