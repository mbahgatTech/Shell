#include "myShell.h"

int main() {
    initShell();
    return 0;
}

void initShell() {
    char **commandPtr = malloc(sizeof(char *));
    commandPtr[0] = malloc(sizeof(char) * 1000);
    char *temp, *buffer = malloc(sizeof(char) * 1000);
    strcpy(*commandPtr, ""); 
    

    pid_t **processes = malloc(sizeof(pid_t *));
    processes[0] = malloc(sizeof(pid_t));
    int length = 0;

    FILE *openFile = NULL;
    int background = 0;
    int outFileNum = -1; // used to revert freopen on stdout
    int inFileNum = -1;// used to revert freopen on stdin
    int pipesEnabled = 0;
    
    // infinite loop waiting for and handling user *commandPtr input
    while(1) {
        // wait for user command input
        printf(">");
        
        // reset command and background flag
        strcpy(*commandPtr, ""); 
        background = 0;
        pipesEnabled = 0;
        outFileNum = dup(fileno(stdout));
        inFileNum = dup(fileno(stdin));

        // take command input
        fgets(*commandPtr, 100, stdin);
        trimString(commandPtr);
        
        // check the type of command and execute accordingly
        if (strlen(*commandPtr) == 0) {
            continue;
        }
        else if (strcmp(*commandPtr, "exit") == 0) {
            freeList(commandPtr, 1);
            free(buffer);
            killShell(processes, length);
        }
        
        // copy command into buffer and tokenize it with spaces
        buffer = realloc(buffer, sizeof(char) * (strlen(*commandPtr) + 1));
        strcpy(buffer, *commandPtr);
        temp = strtok(buffer, " ");
        strcpy(*commandPtr, "");
        
        // loop through all tokens and look for >, < or & tokens
        do {
            if (strcmp(temp, ">") == 0) {
                // next token is file name
                openFile = freopen(strtok(NULL, " "), "w", stdout);
                break;
            }
            else if (strcmp(temp, "<") == 0) {
                // next token is input file name
                openFile = freopen(strtok(NULL, " "), "r", stdin);
                break;
            }
            else if (strcmp(temp, "&") == 0) {
                // assumed to be last argument (turn on background processes)
                background = 1;
                break;
            }
            else if (strcmp(temp, "|") == 0) {
                // tur pipes flag on
                pipesEnabled = 1; 
            }

            strcat(*commandPtr, temp);
            strcat(*commandPtr, " ");
        } while (temp = strtok(NULL, " "));
        
        // remove preceding and trailing spaces then execute command
        trimString(commandPtr);

        if (pipesEnabled == 1) {
            pipeCommand(*commandPtr, processes, &length);
        }
        else {
            forkProcess(*commandPtr, 1, background, processes, &length); 
        }

        // reset both standard streams
        dup2(outFileNum, STDOUT_FILENO);
        dup2(inFileNum, STDIN_FILENO);
    }
}

void trimString(char **ptr) {
    int i = 0;
    int preceded = 0;

    // remove leading spaces in the string
    for(i = 0; i < strlen(*ptr); i++) {
        // set preceded if string begins with space
        if (isspace((*ptr)[i])) {
            preceded = 1;
        }
        else if (preceded == 1) { 
            // trim string with an offset of i (number of spaces) when char is found
            strcpy(*ptr, *ptr + sizeof(char) * i);
            break; // exit loop when a character is found
        }
        else {
            break; // entered when string doesnt have preceding chars
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

void forkProcess(char *command, int currentDir, int background, pid_t **processes, int *length) {
    // check for null command 
    if (command == NULL) {
        return;
    }

    // get command name without args
    int paramNum = 0;
    char **parameters = getParams(command, &paramNum);

    pid_t newProcess = fork(); // returns child pid
    int status;

    if (newProcess < 0) {
        perror("fork");
        return;
    }
    
    // parent process
    if (newProcess > 0) {
        // return parent without waiting for child
        if (background == 1) {
            freeList(parameters, paramNum);
            
            // add pid to the processes array and increment length
            if (processes != NULL && processes[0] != NULL && length != NULL) {
                processes[0] = realloc(processes[0], sizeof(pid_t) * ((*length) + 1));
                processes[0][(*length)++] = newProcess;
            }
            return;
        }

        // block parent process till child completes execution
        waitpid(newProcess, &status, 0);
        freeList(parameters, paramNum);
    }
    else {
        // execute program
        status = execvp(parameters[0], parameters);

        if(status == -1) {
            perror("execvp");
        }
        
        freeList(parameters, paramNum);
        // signal that process ended
        kill(getppid(), SIGCHLD);
    }

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
                // > < arguments arent apart of the command and
                // will be handled by initShell function 
                if (strcmp(params[*length], ">") == 0 || 
                    strcmp(params[*length], "<") == 0) {
                    // ommit the argument and break out of loop
                    free(params[*length]);
                    break;
                }
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

    // remove & from end of the command if present
    if (strcmp(params[(*length) - 1], "&") == 0) {
        // ommit the argument and decrement length
        free(params[--(*length)]);
    }
    
    return params;
}

void pipeCommand(char *command, pid_t **processes, int *length) {
    // check for null command 
    if (command == NULL) {
        return;
    }
    
    // split command into 2 commands (before and after |)
    char *beforeCommand;
    char *afterCommand;
    getCommands(command, 1, &beforeCommand);
    getCommands(command, 0, &afterCommand);
    
    // remove trailing spaces
    trimString(&beforeCommand);
    trimString(&afterCommand);

    // get parameters of both commands
    int firstNum = 0;
    char **firstParams = getParams(beforeCommand, &firstNum);
    int secondNum = 0;
    char **secondParams = getParams(afterCommand, &secondNum);
    
    int outFileNum = dup(fileno(stdout));
    int inFileNum = dup(fileno(stdin));

    // initialize communication pipe
    int commPipe[2];
    pipe(commPipe);

    pid_t newProcess = fork(); // returns child pid
    int status;

    if (newProcess < 0) {
        perror("fork");
        return;
    }
    
    // child process should execute first command and send it to parent
    if (newProcess > 0) { 
        close(commPipe[1]);
        dup2(commPipe[0], STDIN_FILENO);
        close(commPipe[0]);

        char inputString[500];
        status = execvp(secondParams[0], secondParams);
        while(read(STDIN_FILENO, inputString, sizeof(char) * 500) > 0) {
            dup2(outFileNum, STDOUT_FILENO);
        }
        
        free(beforeCommand);
        free(afterCommand);

        freeList(firstParams, firstNum);
        freeList(secondParams, secondNum);
    }
    else {
        close(commPipe[0]);
        dup2(commPipe[1], STDOUT_FILENO);
        close(commPipe[1]);
        status = execvp(firstParams[0], firstParams);

        if (status == -1) {
            perror("execvp");
        }
        
        free(beforeCommand);
        free(afterCommand);
        freeList(firstParams, firstNum);
        freeList(secondParams, secondNum);
        // signal that process ended
        kill(getppid(), SIGCHLD);
    }
}

void getCommands(char *command, int before, char **target) {
    if (command == NULL) {
        return;
    }
    
    char *buffer = malloc(sizeof(char) * (strlen(command) + 1));
    char *temp;
    
    *target = malloc(sizeof(char));
    strcpy(*target, "");
    strcpy(buffer, command);
    temp = strtok(buffer, " ");
    
    do {
        // check if call requires second command when
        // we hit a "|" token, resets target when entered
        if (strcmp(temp, "|") == 0 && before == 0) {
            strcpy(*target, "");
        }
        else if (strcmp(temp, "|") == 0) { 
            // means we needed first command and we already got it
            free(buffer);
            return;
        }
        
        // concatenate each word of the command to target
        *target = realloc(*target, sizeof(char) * (strlen(*target) + strlen(temp) + 2));
        strcat(*target, temp);
        strcat(*target, " ");
    } while (temp = strtok(NULL, " "));

    free(buffer);
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


void killShell(pid_t **children, int length) {
    printf ("myShell terminating...\n");
    
    int killVal = -1;
    
    // kill all child processes
    for (int i = 0; i < length; i++) {
        if (children != NULL && children[0][i]) {
            killVal = kill(children[0][i], SIGKILL);
        }

        // report error tryin to kill a process
        if (killVal == -1) {
            perror("kill");
        }
    }

    printf("\n[Process completed]\n");
    free(children[0]);
    free(children);
    exit(EXIT_SUCCESS);
}