#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/wait.h> 
#include <errno.h> 
#include <signal.h>

// Function initiates the shell and handles all commands given to the program
void initShell();

// Function removes leading and trailing spaces in a string
void trimString(char **ptr);

// Function parses the given command and executes it
void parseCommand(char **commandPtr, pid_t **processes, int *length);

// Function performs export command.
void exportVar(char *command);

// Function prepends the path of the directory that contatins the file for the
// specified command.
void pathPrefix(char **commandPtr, char *temp, char *pathString);

// Function starts a new specified process
void forkProcess(char **command, int background, pid_t **processes, int *length);

// Function returns an array of strings representing parameters in the command string
char **getParams(char *command, int *length);

// Function executes pipe commands
void pipeCommand(char **command, pid_t **processes, int length);

// Function splits the command into 2 commands before and after the | symbol and
// returns the requested command into target indicated by parameter before
void getCommands(char *command, int before, char **target);

// Function frees an array of strings given the length
void freeList(char **list, int length);

// Function kills all processes and ends program exits program
void killShell(pid_t **children, int length);

