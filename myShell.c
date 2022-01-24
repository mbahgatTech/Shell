#include "myShell.h"

int main() {
    initShell();
    return 0;
}

void initShell() {
    char **commandPtr = malloc(sizeof(char *));
    *commandPtr = NULL;

    char *ptr2 = *commandPtr;
    pid_t *processes = malloc(sizeof(pid_t));
    int length = 0;


    // infinite loop waiting for and handling user *commandPtr input
    while(1) {
        // wait for user *commandPtr input
        printf(">");
        
        // reset string to ensure we have 100 chars 
        // because removing preceding memory reduces memory
        if (*commandPtr != NULL) {
            free(*commandPtr);
        }
        *commandPtr = malloc(sizeof(char) * 100);

        strcpy(*commandPtr, ""); // reset string
        fgets(*commandPtr, 100, stdin);
        trimString(commandPtr);
        
        // check the type of *commandPtr and execute accordingly
        if (strlen(*commandPtr) == 0) {
            continue;
        }
        if (strcmp(*commandPtr, "exit") == 0) {
            freeList(commandPtr, 1);
            killShell(processes, length);
        }

        forkProcess(*commandPtr, 1); 
    }
}

void trimString(char **ptr) {
    int i = 0;

    // remove leading spaces in the *ptr
    for(i = 0; i < strlen(*ptr); i++) {
        // increment *ptr pointer to remove the space from beginning of *ptr
        // we decrement i because we moved up in memory by 1 index using our pointer
        if (isspace((*ptr)[i])) {
            (*ptr)++;
            i--;
            // printf("address: %p\n", *ptr);
        }
        else {
            break; // exit loop when a character is found
        }
    }
    
    // remove trailing spaces from *ptr
    i = strlen(*ptr) - 1;
    while (i >= 0) {
        if (!isspace((*ptr)[i])) {
            i--;
            break; // exit loop when a character is found
        }
    
        // make character null if its a space 
        (*ptr)[i] = '\0';
        i--;
    } 
}

void forkProcess(char *command, int currentDir) {
    // check for null command 
    if (command == NULL) {
        return;
    }

    // get command name without args
    int paramNum = 0;
    char **parameters = getParams(command, &paramNum);
    
    pid_t newProcess = fork();
    int status;

    if (newProcess < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    // block parent process till child completes execution
    if (newProcess > 0) {
        waitpid(newProcess, &status, 0);
    }
    else {
        // execute program
        status = execvp(parameters[0], parameters);

        if(status == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    freeList(parameters, paramNum);
}

char **getParams(char *command, int *length) {
    if (command == NULL || length == NULL) {
        return NULL;
    }

    char **params = malloc(sizeof(char *));
    int spacePreceded = 1; // the next character follows a space if set to 1. 
    *length = 0; // num of parameters
    int paramLen = 0;

    for (int i = 0; i < strlen(command); i++) {
        // check if current char is a space and set spacePreceded to 1 
        // and ends the last parameter with a NULL character and reallocates its memory
        if (isspace(command[i])) {
            // only edit params if it not an extra space
            if (spacePreceded != 1 && params[*length] != NULL) {
                params[*length] = realloc(params[*length], sizeof(char) * (strlen(params[*length]) + 1));
                (*length)++; // num of params incremented after last one's assigned
            }

            spacePreceded = 1;
            continue;
        }
        
        // branch checks if this is a new parameter and initializes a string for it
        if (spacePreceded == 1) {
            params = realloc(params, sizeof(char *) * ((*length) + 1));
            params[*length] = malloc(sizeof(char) * (strlen(command) + 1));
            strcpy(params[*length], "");
            paramLen = 0;
            spacePreceded = 0;
        }
        params[*length][paramLen++] = command[i];
        params[*length][paramLen] = '\0';

        // check if last char in the string and set null character in the end of it 
        if(strlen(command) == (i + 1)) {
            params[*length] = realloc(params[*length], sizeof(char) * (strlen(params[*length]) + 1));
            (*length)++;
        }
    }
    
    return params;
}

void freeList(char **list, int length) {
    if (list == NULL) {
        return;
    }

    // free each string
    for (int i = 0; i < length; i++) {
        if (list[i] != NULL) {
            free(list[i]);
        }
    }   
    free(list);
}


void killShell(pid_t *children, int length) {
    printf ("myShell terminating...\n");
    
    int killVal = -1;

    // kill all child processes
    for (int i = 0; i < length; i++) {
        killVal = kill(children[i], SIGKILL);
        
        // print system error and exit
        if (killVal == -1) {
            perror("kill");
            free(children);
            exit(EXIT_FAILURE);
        }
    } 

    printf("[Process completed]\n");
    free(children);
    exit(EXIT_SUCCESS);
}