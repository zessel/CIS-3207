
NAME
    myshell - a shell (command interpreter) for CIS 3207

SYNOPSIS
    myshell [argument]

DESCRIPTION
    myshell is a program that emulates the functions of a BASH or other linux type shells.  While not as feature rich as some shells it has some core features.  
    
BASIC USAGE
    To use the shell simply execute the myshell file.  You should be presented with a prompt of your current directory followed by  "myshell> " in red.  type the commands you would like to use along with their arguments into the primpt

 Arguments
    The shell takes one optional argument only, the name of a text file.
    If run with this argument the shell will not present a prompt and instead execute through the commands of the file in the order they are written, one large batch.
    To be properly parsed by the shell the batch file file must be formatted so that each command is followed by a return character.

 Commands
    myshell has 9 built in commands that is can execute

    1) cd [argument]
        the cd command will print your current directory.
        
        if supplied with the path of another directory in argument
        it will change your current working directory to that directory.

    2) clr
        clears the terminal of previous output

    3) dir [argument]
        prints the contents of the current directory

        if supplied with the path to another directory it will print it's contents instead

    4) environ
        prints the environmental variables

    5) echo [arguments...]
        prints the arguments

    6) help
        displays this text file within the shell using the more
        paging program.

    7) pause
        halts the activity of the shell until return is pressed

    8) quit
        exits the shell

    9) exit
        exits the shell

    10) path [arguments...]
        sets the environment variable "PATH" to the directories supplied in arguments. This affects what external programs the shell can launch

 Externel Programs
    The shell can attempt to launch any external program that it can find within it's PATH environment variable directories.  This program will be launched as it's own process, maintaining shell activity

 IO Redirection
    The shell can have it's input and output directed to and from external files with 3 different flags

    <
        usage command < argument
        This will redirect input from the file specified in argument to the command
    
    >
        usage command > argument
        This will allow output from command to be sent directly to the file specified in argument.  If the file does not exist it will be created, if it does exist it will be overwritten

    >>
        usage command >> argument
        This will allow output from command to be sent directly to the file specified in argument.  If the file does not exist it will be created, if it does exist it will be appended

 Piping
    The shell supports piping between two different commands using the proper flag

    |
        usage command1 [arguements...] | command2 [arguments]
        This will send the output generated from command1 and it's arguments to be the input for command2 and it's arguments

 Background Processing
    The shell supports the execution of commands and return to the shell without waiting for them to finish

    &
        usage command1 & command2 &
        command1 and command2 will be executed in the background while the shell return to it's prompt and wia the next command

LIMITATIONS
    The shell only takes 128 arguments, which include the command you are calling

    Paths are limited in length to PATH_MAX

    The shell was not designed to handle multiple flags, outside of input ( < ) and output ( >, >>) simulataneously.  While multi argument commands may work such behaviour is considered undefined

    The shell was not designed to handle piping to piping.  output will most likely be innaccurate if using two or more pipes

FILES
    ~/.myshell      the shell itself
    ~/readme.md     this readme file

HISTORY
    Completeed with debatable levels of success for the second programing assignment of CIS 3207 Operating Systems at Temple University

AUTHORS
    Zach Essel, 2019

BUGS
    Batchmode with piping has problems.
    If launched with a batchfile that doesn't end with a return character, the shell often loops indefinitely.
    Batchmode in general is unpredictable, with random input from the stream being recycled into input.  Current version simply tries to avoid the problem after many failed attempts to identify and rectify it.