#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h> // For PATH_MAX
#include <limits.h>
#include <dirent.h>       // for chdir()
#include <errno.h>        // for perror()
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>    // For wait() && open()
#include <sys/wait.h>     // For wait()
#include <sys/stat.h>     // for open()
#include <fcntl.h>        // for open()


#include <unistd.h>

extern char ** environ;

int file_line_count(FILE *stream);
void edit_path(int argsnum, char *argument_array[]);
void execute_command (int argsnum, char *argument_array[]);
void split_pipe_arguments(int argsnum, char *argument_array[], char * right_pipe_argument_array[]);
int recount_args(char *argument_array[]);
void reset_io(int old_input_fd, int old_output_fd);
void reset_input(int old_input_fd);
void reset_output(int old_output_fd);
void redirect_io(int argsnum, char *argument_array[], int *old_input_fd, int *old_output_fd);
int redirect_input(char *filename);
int redirect_output(char *filename, int append);
void shell_prompt_print();
void set_env_parent();
void set_shell();
void run_external_function(int argsnum, char *argument_array[]);
int check_shell_function(int argsnum, char *argument_array[]);
void run_shell_function(int argnum, char *argument_array[], int built_in);
void parser ();
void print_args(int argsnum, char*argument_array[]);
char *read_user_input(FILE *stream, long int *position);
int tokenize(char *input, char* argument_array[]);
int check_args_for(int argsnum, char*argument_array[], char *value);
void quit();
void cd (int argsnum, char* new_directory);
void clr();
void dir (char* directory);
void environs();                            //got errors naming this environ
void echo(int argsnum, char *arguments[]);
void help();
void my_pause();                            //got errors naming this pause



void main (int argc, char *argv[])
{
    set_shell();

    char *argument_array[64];
    char *input;
    int old_input_fd;
    int old_output_fd;
    FILE *stream = NULL;
    int batchfile_lines, run_count;
    long int position = 0;

    if (argc >= 2)
    {
        stream = fopen(argv[1], "r");  // attempt to open batchfile as read only
        if (!stream)
        {
            fprintf(stderr, "Error: batchfile not found\n");
            quit();
        }
        batchfile_lines = file_line_count(stream);  //  count the lines of the file
        fseek(stream, 0, SEEK_SET);                 //  reset the file pointer

    }
    else
    {
        stream = stdin;  // read from stdin in user mode
    }
    
    while (1)  
    {   

        old_output_fd = -1;
        old_input_fd = -1;
        if (argc == 1)
        {
            shell_prompt_print();   //only print the prompt in user mode
        }
    /*    else   //  getline was acting unpredictably, putting together strings from random
        {        //  parts of the stream, not recognizing EOF, this was an early workaround
            if ((run_count++ == batchfile_lines) || (feof(stream)))
                quit();  
        }        
    */
        input = read_user_input(stream, &position);
        int argsnum = tokenize(input, argument_array);
        
        if (argument_array[0] != NULL)
        { 
                redirect_io(argsnum, argument_array, &old_input_fd, &old_output_fd);
                argsnum = recount_args(argument_array);

            if (check_args_for(argsnum, argument_array, "|"))  //Piping has a special section of main
            {
                int pipe_ends[2];                              //Create the array for the ends of the pip 0 is read 1 is write
                int pid_right, pid_left;                       //Create pid variables that are to be used in the two upcoming forks 
                char *right_pipe_argument_array[argsnum];      //Create a new array to hold the right side of the pipe arguments
                int status_left, status_right;

                split_pipe_arguments(argsnum, argument_array, right_pipe_argument_array);
                                                    //divides the original arguments into two seperate for both sides of the pipe
                if(pipe(pipe_ends) < 0)
                {                                          //creates the pipe and exits on failure
                    perror("Pipe creation error");
                    quit();
                }                               
                if ((pid_left = fork()) == 0)              //This creates a child for the left hand side. The write end
                {   
                    set_env_parent();                    
                    argsnum = recount_args(argument_array);     //recalculates the number of arguments for left
                    dup2(pipe_ends[1], STDOUT_FILENO);          //redirects stdout to the pipe
                    close(pipe_ends[0]);                        //closes read end of the pipe
                    close(pipe_ends[1]);                        //closes the write end of the pipe

                    execute_command(argsnum, argument_array);   //execute a built in or external command
                    quit();                                     //this should close the child process if it was a built in command
                }
                else if (pid_left < 0)                     //exit if fork creation failed
                {
                    perror("Left pipe fork failed");
                    quit();
                }
                if ((pid_right = fork()) == 0)             //This child will be the right hand side
                {                                                 //this means this side listens to the pipe
                    set_env_parent();
                    argsnum = recount_args(right_pipe_argument_array);  //recount the arguments for just the right hand side
                    dup2(pipe_ends[0], STDIN_FILENO);                   //redirect the pipe read end of the pipe to stdin
                    close(pipe_ends[1]);                                //close the write end of the pipe
                    close(pipe_ends[0]);                                //closes the read end of pipe, I think this is needed to prevent a hang
                    execute_command(argsnum, right_pipe_argument_array);//execute a built in or external command
                    quit();                                             //this should close the child process if it was a built in command
                }
                else if (pid_right < 0)                     //exit if fork creation failed
                {
                    perror("Right pipe fork failed");
                    quit();
                }
                    close(pipe_ends[1]);
                    close(pipe_ends[2]);
                    waitpid(pid_left, &status_left, 0);         //wait for the output creating (left) child to terminate
                    waitpid(pid_right, &status_right, 0);       //possibly excessive wait for the input taking (right) child
            }
            else
            {
                execute_command(argsnum, argument_array);
            }
        }
        reset_io(old_input_fd, old_output_fd);
    }
    fprintf(stderr, "You shouldn't have gotten to this point");
}


/*  a function to count the returns in a file

    this was a workaround for the unexplained behavior of getline in
    batchfile mode after a pipe.  Abandoned for ftell() fseek()
*/
int file_line_count(FILE *stream)
{
    char buf[512];
    int count = 0;
    while(fgets(buf,sizeof(buf), stream) != NULL)
    {
        count++;
    }
    return count;
}

/*  function to allow path editing with path command

    unclear whether this was needed, listed in one part of assignment
    requirements and not in another
*/
void edit_path(int argsnum, char *argument_array[])
{
    char new_path_value[PATH_MAX];
    if (argsnum == 1)
        new_path_value[0] = '\0';
    else
    {
        strcpy(new_path_value, argument_array[1]);
        for (int i = 2; i < argsnum; i++)
        {
            strcat(new_path_value, ":");
            strcat(new_path_value, argument_array[i]);
        }
    }
    setenv("PATH", new_path_value, 1);
    
}

/*  Checks if a command is build into the shell and executes either way

    This code block ended up repeating 3 times in main so it
    got moved into it's own function
*/
void execute_command (int argsnum, char *argument_array[])
{
    int built_in = check_shell_function(argsnum, argument_array);

    if (built_in > 0)
    {
        run_shell_function(argsnum, argument_array, built_in);
    }
    else
    {
        run_external_function(argsnum, argument_array);
    }
}

void split_pipe_arguments(int argsnum, char *argument_array[], char * right_pipe_argument_array[])
{
    int hit_pipe = 0;    
    for (int i = 0; i < argsnum; i++)
    {
        if (hit_pipe != 0)
            right_pipe_argument_array[i-hit_pipe-1] = argument_array[i];
        if (strcmp(argument_array[i], "|") == 0)
        {
            hit_pipe = i;
            argument_array[i] = NULL;
        }
    }
    right_pipe_argument_array[argsnum-hit_pipe-1] = NULL;
}

/*  This function counts the number of arguments in the array
    
    The redirect_io function has the ability to NULL pointers in the
    arguments array after a redirect has been processed.  This was causing
    seg faults later on for other functions that required an accurate number
    of arguments.
*/
int recount_args(char *argument_array[])
{
    int i = 0;
    while (argument_array[i] != NULL)
        i++;
    return i;
}

/*  reset_io checks if the input was changed before calling
    a funciton to reset it back to the original

    because file descriptors are always returned positive by initializing
    these as negative in main you can use them as sentinel values 
*/
void reset_io(int old_input_fd, int old_output_fd)
{
    if (old_input_fd != -1)
        reset_input(old_input_fd);
    if (old_output_fd != -1)
        reset_output(old_output_fd);
}

/*  Resets the input back to the original file descriptor
*/
void reset_input(int old_input_fd)
{
    dup2(old_input_fd, STDIN_FILENO);
    close(old_input_fd);
}

/*  Resets the output back to the original file descriptor
*/
void reset_output(int old_output_fd)
{
    dup2(old_output_fd, STDOUT_FILENO);
    close(old_output_fd);
}

/*  search backwards through the arguments until you hit a redirect
    then go forward once to grab the file name and call a function
    to perform the redirect

    The search is backwards so that you can NULL any string pointer
    that contained a redirect, effective "erasing" it from the arguments.
*/
void redirect_io(int argsnum, char *argument_array[], int *old_input_fd, int *old_output_fd)
{   
    int input_flag = check_args_for(argsnum, argument_array, "<");
    int output_overwrite_flag = check_args_for(argsnum, argument_array, ">");
    int output_append_flag = check_args_for(argsnum, argument_array, ">>");
    
    for (int i = (argsnum - 1); i > 0; i--)
    {   if (i == output_append_flag)
        {
            *old_output_fd = redirect_output(argument_array[i+1], 1);
            argument_array[i] = NULL;
        }
        else if (i == output_overwrite_flag)
        {
            *old_output_fd = redirect_output(argument_array[i+1], 0);
            argument_array[i] = NULL;
        }
        else if (i == input_flag)
        {
            *old_input_fd = redirect_input(argument_array[i+1]);
            argument_array[i] = NULL;
        }
    }
}

/*  redirects input to the file in filename

    Makes a copy of the original file descriptor with dup
    opens the filename file read only
    redirects standard input to the filename file
    closes the file
    returns the original standard input copy so that redirect can be reversed
*/
int redirect_input(char *filename)
{
    int old_input_fd = dup(STDIN_FILENO);
    int new_input_fd = open(filename, O_RDONLY);
    dup2(new_input_fd, STDIN_FILENO);
    close(new_input_fd);
    return old_input_fd;
}

/*  changes the file descriptors to redirect output
    sets permission to 666 which is read write for all levels
    supposedly the standard but a variable is set up for quick editing
    opposed to hardcoding the 666 into the mode argument of open()\
    
    After redirecting the output with dup2() the file is closed

    Since both output redirects are almost identical with the exception of
    O_APPEND or O_TRUNC the two different redirects are combined into one function
    with the bool append denoting the difference

    returns the old file descriptor so that input can be reverted back
*/
int redirect_output(char *filename, int append)
{
    mode_t wrmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int old_output_fd = dup(STDOUT_FILENO);
    int new_output_fd;
    if (append == 1)
        new_output_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, wrmode);
    else
        new_output_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, wrmode);

    dup2(new_output_fd, STDOUT_FILENO);
    close(new_output_fd);
    return (old_output_fd);
}

/*  prints the command line prompt for the shell

    colors were added originally to differenciate the prompt from
    poorly formatted output.  However, it gave the shell a more authentic
    feel, so username was added as well
*/
void shell_prompt_print()
{
        printf("\x1b[96m");
        cd(1, NULL);
        printf("\x1b[31m myshell> \x1b[0m");
}

/*  Set a variable called PARENT in the environment

    called in both forks and also in run external
*/
void set_env_parent()
{
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, "/myshell");
    setenv("PARENT", cwd, 1);
}
/*  Changes two environmental variables

    The first is it overwrites the directory of the shell you called
    myshell in with the directory that myshell was called from
    ie ../bash -> ../desktop/myshell 
    
    The second is used to keep a path to the help file so it can be called
    if you have left the directory you started in.  Maybe environmental
    variables are not meant to be used this way but it seems like a clean work
    around
*/
void set_shell()
{
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    strcat(cwd, "/myshell");
    setenv("SHELL", cwd, 1);

    char actualpath [PATH_MAX];
    realpath("readme.md", actualpath);
    setenv("README", actualpath, 1);
}

/*  function for running any non built-in command

    forks a child process
    overwrites that child process with execvp
    -p to search for the program
    -l because it is passed the argument_array

    then parent process will wait or the child to finish unless
    a & was detected
*/
void run_external_function(int argsnum, char *argument_array[])
{
    int background_flag = check_args_for(argsnum, argument_array, "&");
    int status;

    int pid = fork();
    if (pid == 0)
    {
        set_env_parent();

        if (execvp(argument_array[0], argument_array) == -1)
        {
            fprintf(stderr, "Could not execute '%s'", argument_array[0]);
            perror(" ");
            quit();
        }                     
        else
        {
            fprintf(stderr, "%s: command not found\n", argument_array[0]);
        }
        
    }
    else if (!background_flag)
        waitpid(pid, &status, 0);
}

/*  NOT USED Attempts to run any program not predefined in the shell

    Checks for execution access on the /bin/command and /usr/bin/command paths
    if found attempts to fork and execv

    There is a confusing requirement that I didn't notice until reviewing my program
    It states that only execv can be used, meaning the arg[0] program won't be searched for
    instead you have to provide a path the the executable.  I'm not sure how many paths
    are supposed to be checked, it seems like maybe only bin, but I added usr/bin as well
    because the tr program was very useful for checking piping.

    Checking again the TA recommends using execvp so maybe this just meant we had to use pointer
    arrays, not strings.

    This function is not used, but included as proof of project requirements
    I prefer the function "run_external_function" because it's cleaner
*/
void run_external_function_exec(int argsnum, char *argument_array[])
{
    int background_flag = check_args_for(argsnum, argument_array, "&");
    int status;

    int pid = fork();
    if (pid == 0)
    {
        char program_path[6+sizeof(argument_array[0])];
        strcpy(program_path, "/bin/");
        strcat(program_path, argument_array[0]);

        char program_path_usr[10+sizeof(argument_array[0])];
        strcpy(program_path_usr, "/usr/bin/");
        strcat(program_path_usr, argument_array[0]);

        set_env_parent();
        if (access(program_path, X_OK) == 0)
        {
            if (execv(program_path, argument_array) == -1)
            {
                fprintf(stderr, "Could not execute '%s'", argument_array[0]);
                perror(" ");
                quit();
            }            
        }
        else if (access(program_path_usr, X_OK) == 0)
        {
            if (execv(program_path_usr, argument_array) == -1)
            {
                fprintf(stderr, "Could not execute '%s'", argument_array[0]);
                perror(" ");
                quit();
            }            
        }
        else
        {
            fprintf(stderr, "%s: command not found\n", argument_array[0]);
        }
        
    }
    else if (!background_flag)
        wait(&status);
}

/*  Checks if a function built into the shell is called at argument_array[0] 
    returns an identified for that function

    had to change to strncmp because strcmp was only working if the
    command was followed by whitespace.  I imagine there is a difference
    in null char or \n but this should be a fine workaround

    There is also support for "exit" which just executes as quit
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
    else if (strncmp(argument_array[0], "exit", 4) == 0)
        is_function = 8;
    else if (strncmp(argument_array[0], "path", 4) == 0)
        is_function = 9;

    return is_function;
}

/*  run_shell_function will run the proper built in function specified in the
    command

    Seperate from the logic of check_shell_function mostly just 
    to make it look slightly nicer
*/
void run_shell_function(int argsnum, char *argument_array[], int built_in)
{
    switch(built_in)
    {
        case 1: 
            cd(argsnum, argument_array[1]);
            printf("\n");
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
        case 9: edit_path(argsnum, argument_array);
            break;
    }
}

/*  an early function that ran parsing operations for testing

    not called
*/
void parser ()
{
    char *input = read_user_input(NULL, 0);
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
    printf("\n");
}

/*  This will return the users input stream as a string.  
    If input and length are NULL and 0 getline handles the buffer allocation, user handles the free

    In batchfile mode this function has unpredictable behaviour, adding previous parts of the 
    stream to the input string after encountering a pipe

    added position tracking to work around.  Records position after a getline and offsets
    it from the start of stream before next getline.

    perror will print "bad file descriptor" after any piping call
*/
char *read_user_input(FILE * stream, long int *position)
{
    char *input = NULL;
    size_t length = 0;
    fseek(stream, *position, SEEK_SET);
    if (getline(&input, &length, stream) == -1)
        quit();
    *position = ftell(stream); 
    // perror("stream");

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
            hit_value = i;
    }
    return hit_value;
}

/* Quits shell*/
void quit()
{
    exit(0);
}

/* calls chdir to change the directory to a given path
   TODO Add proper error printing if time permits

   There are conflicting specifications on what the cd function
   must do.  It must both error out on receiving no arguments
   and also print of the current directory

   I chose to go with the current directory.

   To error would just involve replacing the two cwd commands
   with a fprintf to stderr
*/
void cd (int argsnum, char* new_directory)
{
    int error;
    if (new_directory == NULL)
    {
        char cwd[PATH_MAX];
        printf("%s", getcwd(cwd, sizeof(cwd)));
    }
    else if (argsnum > 2)
    {
        fprintf(stderr, "cd: too many arguments");
    }
    else 
    {
        error = chdir(new_directory);
        if (error != 0)
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

/*  This help forks a child that runs the more command with the readme

    execlp is used because it was easier to hardcode the arguments in as strings
    with l and the more command will be searched for with p

    changed this to execvp because I saw that execv must always be used in this project
*/
void help()
{
    int pid = fork();
    int status;
    char *help_args[3];
    help_args[0] = "/bin/more";
    help_args[1] = getenv("README");
    help_args[2] = NULL;
    if (pid == 0)
    {   
        execv(help_args[0], help_args);
        //execlp("more", "more", getenv("README"), (char *) NULL);
    }
    else
    {
        wait(&status);
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

    UPDATE - Reading the project requirements I think I was only supposed to actually call
    more command, not try to implement my own so I have this depreciated
 */
void help_depreciated()
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
