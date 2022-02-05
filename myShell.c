#include "myShell.h"

int main() {
    initShell();
    return 0;
}

void initShell() {
    char *profiles = malloc(sizeof(char) * (strlen(getenv("HOME")) + strlen("/.CIS3110_history") + 1));
    strcpy(profiles, getenv("HOME"));
    strcat(profiles, "/.CIS3110_history");

    // set environment variables on startup
    setenv("myPath", "/bin", 1);
    setenv("myHISTFILE", profiles, 1);
    setenv("myHOME", getenv("HOME"), 1);

    char **commandPtr = malloc(sizeof(char *));
    commandPtr[0] = malloc(sizeof(char) * 1000);
    strcpy(*commandPtr, ""); 

    pid_t **processes = malloc(sizeof(pid_t *));
    processes[0] = malloc(sizeof(pid_t));
    int length = 0;

    
    // open profile file and execute its commands
    FILE *openFile = NULL;
    strcpy(profiles, getenv("HOME"));
    strcat(profiles, "/.CIS3110_profile");
    openFile = fopen(profiles, "r");
    free(profiles);
    

    while (openFile && !feof(openFile)) {
        // take each command from a line in the file untill end of file
        fgets(*commandPtr, 100, openFile);
        trimString(commandPtr);
        
        // profile shoudlnt have an exit command if it wants to allow
        // user given commands
        if (strcmp(*commandPtr, "exit") == 0) {
            freeList(commandPtr, 1);
            killShell(processes, length);
        }

        parseCommand(commandPtr, processes, &length);
    }

    if (openFile) {
        fclose(openFile);
    }
    
    // infinite loop waiting for and handling user *commandPtr input
    while(1) {
        profiles = getcwd(NULL, 100);
        printf("%s> ", profiles);
        free(profiles);
        *commandPtr = realloc(*commandPtr, sizeof(char) * 100);
        strcpy(*commandPtr, ""); 

        // take command input
        fgets(*commandPtr, 100, stdin);
        trimString(commandPtr);
        
        // write command to history file
        openFile = fopen(getenv("myHISTFILE"), "a");
        if (openFile) {
            fprintf(openFile, "%s\n", *commandPtr);
            fclose(openFile);
        }
        else {
            // open for writing if append failed, meaning file doesnt exist
            openFile = fopen(getenv("myHISTFILE"), "w");
            if (openFile) {
                fprintf(openFile, "%s\n", *commandPtr);
                fclose(openFile);
            }
        }
        
        // check the command is exit and kill shell if entered
        if (strcmp(*commandPtr, "exit") == 0) {
            freeList(commandPtr, 1);
            killShell(processes, length);
        }
        parseCommand(commandPtr, processes, &length);
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

void parseCommand(char **commandPtr, pid_t **processes, int *length) {
    // no command given
    if (!commandPtr || !*commandPtr || strlen(*commandPtr) == 0) {
        return;
    }
    
    char *temp, *buffer;
    int background = 0, pipesEnabled = 0;
    int outFileNum = dup(STDOUT_FILENO);
    int inFileNum = dup(STDIN_FILENO);
    FILE *openFile, *openReadFile;

    
    // copy command into buffer and tokenize it with spaces
    buffer = malloc(sizeof(char) * (strlen(*commandPtr) + 1));
    strcpy(buffer, *commandPtr);
    temp = strtok(buffer, " ");
    strcpy(*commandPtr, "");
    
    int i = 0;
    // check the type of command and execute accordingly 
    do {
        // enters on cd command and takes next argument as directory
        if (i == 0 && strcmp(temp, "cd") == 0 && (temp = strtok(NULL, " ")) != NULL) {
            if (chdir(temp) != 0) {
                perror("cd");
            }
            free(buffer);
            return;
        }
        else if(i == 0 && strcmp(temp, "export") == 0) {
            /* 
            second parameter should be in the following format:
            envar=val1:val2:val3
            */ 
            if((temp = strtok(NULL, " ")) != NULL) {
                char *temp2 = strtok(temp, "=");
                setenv(temp2, strtok(NULL, "="), 1);
            }
            free(buffer);
            return;
        } 
        // history branch opens myHISTFILE and outputs its contents
        else if (i == 0 && strcmp(temp, "history") == 0) {
            openFile = fopen(getenv("myHISTFILE"), "r");
            if (!openFile) {
                free(buffer);
                return;
            }
            
            // print every line in the file in the requested format
            int counter = 1;
            while(!feof(openFile)) {
                // check for parameters
                if ((temp = strtok(NULL, " ")) != NULL) {
                    // clear file by opening it for writing
                    if (strcmp(temp, "-c") == 0) {
                        fclose(openFile);
                        openFile = fopen(getenv("myHISTFILE"), "w");

                        fclose(openFile);
                        openFile = fopen(getenv("myHISTFILE"), "r");
                        continue;
                    }
                }
                
                // reuse temp as a buffer for history file commands
                temp = malloc(sizeof(char) * 100);
                strcpy(temp, "");
                fgets(temp, 100, openFile);
                
                if (strlen(temp) > 0) {
                    // remove new line at the end of temp before printing
                    if (temp[strlen(temp) - 1] == '\n') {
                        temp[strlen(temp) - 1] = '\0';
                    }

                    printf(" %d  %s\n", counter++, temp);
                }
                free(temp);
            }
            fclose(openFile);
            free(buffer);
            return;
        }
        
        if (i == 0 && temp[0] && temp[0] != '.') {
            pathPrefix(commandPtr, temp, getenv("myPath"));
        }


        // loop through all tokens and look for >, <,  & or | tokens
        if (strcmp(temp, ">") == 0) {
            // next token is file name
            openFile = freopen(strtok(NULL, " "), "w", stdout);
            break;
        }
        if (strcmp(temp, "<") == 0) {
            // next token is input file name
            openReadFile = freopen(strtok(NULL, " "), "r", stdin);
            break;
        }
        if (strcmp(temp, "&") == 0) {
            // assumed to be last argument (turn on background processes)
            background = 1;
            break;
        }
        if (strcmp(temp, "|") == 0) {
            // turn pipes flag on
            pipesEnabled = 1; 
            i = -1;
        }


        strcat(*commandPtr, temp);
        strcat(*commandPtr, " ");
        i++;
    } while (temp = strtok(NULL, " "));
    
    // remove preceding and trailing spaces then execute command
    trimString(commandPtr);

    if (pipesEnabled == 1) {
        pipeCommand(*commandPtr);
    }
    else {
        forkProcess(*commandPtr, background, processes, length); 
    }

    // reset both standard streams
    dup2(outFileNum, STDOUT_FILENO);
    dup2(inFileNum, STDIN_FILENO);

    free(buffer);
}

void pathPrefix(char **commandPtr, char *temp, char *pathString) {
    if (!temp || !pathString || !commandPtr) {
        return;
    }

    // tokenize myPath directories
    char *currPath = malloc(sizeof(char) * (strlen(pathString) + 1));
    strcpy(currPath, "");
    
    // copy first path directory
    int index = 0;
    for (int i = 0; i < strlen(pathString); i++) {
        if (pathString[i] == ':' || isspace(pathString[i])) {
            break;
        } 
        currPath[i] = pathString[i];
        currPath[i + 1] = '\0';
    }

    // prepend current myPath directory to fileName 
    FILE *tempFile;
    char * fileName = malloc(sizeof(char) * (strlen(currPath) + strlen(temp) + 2));
    strcpy(fileName, currPath);
    strcat(fileName, "/");
    strcat(fileName, temp);

    if ((tempFile = fopen(currPath, "r")) != NULL) {
        // copy the right directory name to the command
        *commandPtr = realloc(*commandPtr, sizeof(char) * (100 + strlen(currPath) + 2));
        strcat(*commandPtr, currPath);
        strcat(*commandPtr, "/");

        free(fileName);
        fclose(tempFile);
        free(currPath);
        return;
    }
    
    // copy next path directoy and make a recursive call on it
    strcpy(currPath, "");
    for (int i = strlen(currPath) + 1; i < strlen(pathString); i++) {
        if (pathString[i] == ':' || isspace(pathString[i])) {
            break;
        } 
        currPath[i] = pathString[i];
        currPath[i + 1] = '\0';
    }
    // make sure there are remaining path direcotories
    if (strlen(currPath) > 0) {
        pathPrefix(commandPtr, temp, currPath);
    }
    
    free(currPath);
    free(fileName);
}

void forkProcess(char *command, int background, pid_t **processes, int *length) {
    // check for null command 
    if (command == NULL) {
        return;
    }

    // get command name without args
    int paramNum = 0;
    char **parameters = getParams(command, &paramNum);
    if (!parameters || !parameters[0]) {
        return;
    }

    pid_t newProcess = fork(); // returns child pid
    int status;

    if (newProcess < 0) {
        perror("fork");
        exit (EXIT_FAILURE);
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
        status = execv(parameters[0], parameters);

        if(status == -1) {
            perror("execv");
        }
        
        // signal that process ended
        kill(getppid(), SIGCHLD);
        exit(status);
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
            
            // environment variable replacement
            if (command[i] == '$') {
                // create buffer to get env variable name
                char *envBuffer = malloc(sizeof(char) * (strlen(command) + 1));
                strcpy(envBuffer, "");

                // loop till space or end of command and copy chars to buffer
                int j = i + 1;
                while (!isspace(command[j]) && j != (strlen(command))) {
                    envBuffer[paramLen++] = command[j++];
                    envBuffer[paramLen] = '\0'; 
                }
                
                params[*length] = realloc(params[*length], sizeof(char) * (strlen(getenv(envBuffer)) + 1));
                params[*length] = strcpy(params[*length], getenv(envBuffer));
                free(envBuffer);
                
                // skip the buffer characters that have already been replace in params
                (*length)++;
                i = j;
                continue;
            }
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
    if ((*length) != 0 && strcmp(params[(*length) - 1], "&") == 0) {
        // ommit the argument and decrement length
        strcpy(params[--(*length)], "");
        free(params[*length]);
    }
    
    return params;
}

void pipeCommand(char *command) {
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
        exit(EXIT_FAILURE);
    }
    
    // child process should execute first command and send output to its child
    if (newProcess == 0) {
        // fork child
        pid_t secondProcess = fork();
        
        if (secondProcess < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        // parent process
        if (secondProcess > 0) {
            // copy commPipe[0] of this process into stdin index of file
            // descriptor table 
            close(commPipe[1]);	
            dup2(commPipe[0], STDIN_FILENO);
            close(commPipe[0]);
            
            // execute second command takin input from the child process
            status = execv(secondParams[0], secondParams);

            if (status == -1) {
                perror("execv");
            }

            exit(status);
        }
        else {
            // copy commPipe[1] of the grandchild process into stdout index of 
            // file descriptor table and close unused ends of the pipe. 
            // Both after copying the commPipe[1] 
            close(commPipe[0]);
            dup2(commPipe[1], STDOUT_FILENO);
            close(commPipe[1]);
            
            // execute the first command giving output to parent process
            status = execv(firstParams[0], firstParams);

            if (status == -1) {
                perror("execv");
            }
    
            exit(status);
        }
    }
    else if (newProcess > 0) {  // grandparent process
        // close both ends of the pipe for this process since 
        // since communication isnt needed for the grandparent
        close(commPipe[1]);	
        close(commPipe[0]);

        // waits for both processes to execute and return
        waitpid(newProcess, &status, 0);
        if (status == -1) {
            perror("execv");
        }
        
        // free command and parameter strings
        free(beforeCommand);
        free(afterCommand);
        freeList(firstParams, firstNum);
        freeList(secondParams, secondNum);
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
            continue;
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