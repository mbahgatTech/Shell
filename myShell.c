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

        // write command to history file
        if (strlen(*commandPtr) > 0) {
            FILE *histFileWrite = fopen(getenv("myHISTFILE"), "a");
            if (histFileWrite) {
                fprintf(histFileWrite, "%s\n", *commandPtr);
                fclose(histFileWrite);
            }
            else {
                // open for writing if append failed, meaning file doesnt exist
                histFileWrite = fopen(getenv("myHISTFILE"), "w");
                if (histFileWrite) {
                    fprintf(histFileWrite, "%s\n", *commandPtr);
                    fclose(histFileWrite);
                }
            }
        }
        
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
        if (strlen(*commandPtr) > 0) {
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
            exportVar(strtok(NULL, " "));
            
            // get rid of all tokens
            while((temp = strtok(NULL, " ")) != NULL) {
                continue;
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
            
            // check for parameters
            if ((temp = strtok(NULL, " ")) != NULL) {
                // clear file by opening it for writing
                if (strcmp(temp, "-c") == 0) {
                    fclose(openFile);
                    openFile = fopen(getenv("myHISTFILE"), "w");

                    fclose(openFile);
                    openFile = fopen(getenv("myHISTFILE"), "r");
                }
                else {
                    int digits = 1;
                    // check if all chars are digits in the token
                    for (int idx = 0; idx < strlen(temp); idx++) {
                        if (!isdigit(temp[idx])) {
                            digits = 0;
                        }
                    }
                    
                    if (digits == 1) {
                        outputLast(openFile, atoi(temp));
                    }
                }
            }

            // print every line in the file in the requested format
            int counter = 1;
            while(!feof(openFile)) {
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
            continue;
        }
        else if (strcmp(temp, "<") == 0) {
            // next token is input file name
            openReadFile = freopen(strtok(NULL, " "), "r", stdin);
            continue;
        }
        else if (strcmp(temp, "&") == 0) {
            // assumed to be last argument (turn on background processes)
            background = 1;
            break;
        }
        else if (strcmp(temp, "|") == 0) {
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
    free(buffer);

    if (pipesEnabled == 1) {
        pipeCommand(commandPtr, processes, *length);
    }
    else {
        forkProcess(commandPtr, background, processes, length); 
    }

    // reset both standard streams
    dup2(outFileNum, STDOUT_FILENO);
    dup2(inFileNum, STDIN_FILENO);
}

void outputLast(FILE *file, int n) {
    if (!file) {
        return;
    }
    
    char *buffer = malloc(sizeof(char) * 100);
    int count = 0;
    
    // count the number of lines in the file
    while(!feof(file)) {
        fgets(buffer, 100, file);
        count++;
    }
    count--; // last attempt is on an empty line shouldnt count.

    int i = count - n; // include the last command 
    if (i < 0) {
        i = 0;
    }
    
    // start from beginning of file and skip first i lines
    fseek(file, 0, SEEK_SET);
    for(int offset = 0; offset < i; offset++) {
        fgets(buffer, 100, file);
    }

    // print last n commands
    for(i; i < count && !feof(file); i++) {
        strcpy(buffer, "");
        fgets(buffer, 100, file);
        printf(" %d  %s", i + 1, buffer);
    }

    free(buffer);
}

void exportVar(char *command) {
    if (!command) {
        return;
    }
    
    char *varName = malloc(sizeof(char) * (strlen(command) + 1));
    int i = 0;
    for (i = 0; i < strlen(command); i++) {
        // end of variable name when = sign is encountered
        if (command[i] == '=') {
            i++;
            break;
        }
        
        varName[i] = command[i];
        varName[i + 1] = '\0';
    }
    
    char *varValue = malloc(sizeof(char) * (strlen(command) + 1));
    int index = 0;
    for (i = i; i < strlen(command); i++) {
        // in case of any environment variables in the value are present
        // replace them with the actual value
        if (command[i] == '$') {
            // create buffer to get env variable name
            char *envBuffer = malloc(sizeof(char) * (strlen(command) + 1));
            strcpy(envBuffer, "");

            // loop till space or end of command and copy chars to buffer
            int j = i + 1;
            while (j < strlen(command) && (!isspace(command[j]) || command[j] != '/' || command[j] != ':')) { 
                envBuffer[j - i - 1] = command[j];
                envBuffer[j - i] = '\0'; 
                j++;
            }
            
            char *temp = getenv(envBuffer);

            if (!temp) {
                return;
            }

            varValue = realloc(varValue, sizeof(char) * (strlen(command) + strlen(temp) + 1));
            strcat(varValue, temp);
            index = strlen(varValue);          

            free(envBuffer);
            i = j;
        }

        varValue[index++] = command[i]; 
        varValue[index] = '\0';
    }
    
    // set environment variable to its specified value
    setenv(varName, varValue, 1);
    free(varName);
    free(varValue);
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

    if ((tempFile = fopen(fileName, "r")) != NULL) {
        // copy the right directory name to the command
        *commandPtr = realloc(*commandPtr, sizeof(char) * (100 + strlen(currPath) + 2));
        strcat(*commandPtr, currPath);
        strcat(*commandPtr, "/");

        free(fileName);
        fclose(tempFile);
        free(currPath);
        return;
    }
    
    // copy next path directories and make a recursive call on it
    int lastPathLen = strlen(currPath);
    strcpy(currPath, "");

    index = 0;
    for (int i = lastPathLen + 1; i < strlen(pathString); i++) {
        currPath[index++] = pathString[i];
        currPath[index] = '\0';
    }

    // make sure there are remaining path direcotories
    if (strlen(currPath) > 0) {
        pathPrefix(commandPtr, temp, currPath);
    }
    
    free(currPath);
    free(fileName);
}

void forkProcess(char **command, int background, pid_t **processes, int *length) {
    // check for null *command 
    if (*command == NULL) {
        return;
    }

    // get *command name without args
    int paramNum = 0;
    char **parameters = getParams(*command, &paramNum);
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
            freeList(parameters, paramNum + 1);
            
            // add pid to the processes array and increment length
            if (processes != NULL && processes[0] != NULL && length != NULL) {
                processes[0] = realloc(processes[0], sizeof(pid_t) * ((*length) + 1));
                processes[0][(*length)++] = newProcess;
            }
            return;
        }

        // block parent process till child completes execution
        waitpid(newProcess, &status, 0);
    }
    else {
        // execute program
        status = execv(parameters[0], parameters);

        if(status == -1) {
            perror("execv");
        }
        
        freeList(parameters, paramNum + 1);
        for (int k = 1; k < *length; k++) {
            free(processes[k]);
        }
        free(processes[0]);
        free(processes);
        freeList(command, 1);

        // signal that process ended
        kill(getppid(), SIGCHLD);
        exit(status);
    }

    freeList(parameters, paramNum + 1);
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
            
            // environment variable replacement
            if (command[i] == '$') {
                // create buffer to get env variable name
                char *envBuffer = malloc(sizeof(char) * (strlen(command) + 1));
                strcpy(envBuffer, "");

                // loop till space or end of command and copy chars to buffer
                int j = i + 1;
                while (j < strlen(command) && (!isspace(command[j]) || command[j] != '/' || command[j] != ':')) { 
                    envBuffer[paramLen++] = command[j++];
                    envBuffer[paramLen] = '\0'; 
                }
                
                char *tempEnv = getenv(envBuffer);
                params[*length] = realloc(params[*length], sizeof(char) * (strlen(tempEnv) + 1));
                params[*length] = strcpy(params[*length], tempEnv);
                free(envBuffer);
                
                // skip the buffer characters that have already been replace in params
                if (j == strlen(command)) {
                    (*length)++;
                }
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
    if ((*length) != 0 && (strcmp(params[(*length) - 1], "&") == 0)) {
        // ommit the argument and decrement length
        strcpy(params[--(*length)], "");
        free(params[*length]);
    }

    params = realloc(params, sizeof(char *) * ((*length) + 1));
    params[*length] = NULL;
    
    return params;
}

void pipeCommand(char **command, pid_t **processes, int length) {
    // check for null *command 
    if (*command == NULL) {
        return;
    }
    
    // split *command into 2 *commands (before and after |)
    char *beforeCommand; 
    char *afterCommand;
    getCommands(*command, 1, &beforeCommand);
    getCommands(*command, 0, &afterCommand);
    
    // remove trailing spaces
    trimString(&beforeCommand);
    trimString(&afterCommand);

    // get parameters of both *commands
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
    
    // child process should execute first *command and send output to its child
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
            
            // execute second *command takin input from the child process
            status = execv(secondParams[0], secondParams);

            if (status == -1) {
                perror("execv");
            }
            
            // avoid mem leaks when program encounters a problem
            free(beforeCommand);
            free(afterCommand);
            freeList(firstParams, firstNum + 1);
            freeList(secondParams, secondNum + 1);
            freeList(command, 1);

            for (int i = 1; i < length; i++) {
                free(processes[i]);
            }
            free(processes[0]);
            free(processes);

            exit(status);
        }
        else {
            // copy commPipe[1] of the grandchild process into stdout index of 
            // file descriptor table and close unused ends of the pipe. 
            // Both after copying the commPipe[1] 
            close(commPipe[0]);
            dup2(commPipe[1], STDOUT_FILENO);
            close(commPipe[1]);
            
            // execute the first *command giving output to parent process
            status = execv(firstParams[0], firstParams);

            if (status == -1) {
                perror("execv");
            }

            free(beforeCommand);
            free(afterCommand);
            freeList(firstParams, firstNum + 1);
            freeList(secondParams, secondNum + 1);
            freeList(command, 1);

            for (int i = 1; i < length; i++) {
                free(processes[i]); 
            }
            free(processes[0]);
            free(processes);
    
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
        
        // free *command and parameter strings
        free(beforeCommand);
        free(afterCommand);
        freeList(firstParams, firstNum + 1);
        freeList(secondParams, secondNum + 1);
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
        free(list[i]);
    }   
    free(list);
}


void killShell(pid_t **children, int length) {
    printf ("myShell terminating...\n");
    
    int killVal = -1;
    
    // kill all child processes
    for (int i = 1; i < length; i++) {
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