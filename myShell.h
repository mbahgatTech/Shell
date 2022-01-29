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

// Function starts a new specified process
void forkProcess(char *command, int currentDir, int background, pid_t **processes, int *length);

// Function returns an array of strings representing parameters in the command string
char **getParams(char *command, int *length);

// Function executes 2 commands seperated by '|' where output of first 
// command is piped to the input of the second command. 
void pipeCommand(char *command, pid_t **processes, int *length);

// Function gets command before or after '|' token from command string and copies it into *target
void getCommands(char *command, int before, char **target);

// Function frees an array of strings given the length
void freeList(char **list, int length);

// Function kills all processes and ends program exits program
void killShell(pid_t **children, int length);

