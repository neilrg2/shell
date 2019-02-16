#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LENGTH              2048
#define MAX_ARGS                512
#define COMMENT                 '#'
#define PID_EXPANSION           "$$"
#define BACKGROUND_PROCESS      "&"
#define STATUS                  "status\n"
#define EXIT                    "exit\n"


int main(int argc, const char *argv[])
{
    pid_t spawnChild = 0;
    size_t MAX = MAX_LENGTH;
    
    
    const int PPID = (int)getpid();
    const char DELIMITER[3] = " \n";
    
    char *token;
    char *tokenBuffer;
    char *args[MAX_ARGS];
    char *commandLineArgs = (char *)malloc(sizeof(char) * MAX_LENGTH);
    
    int childExitStatus = 0;
    int exitMethod;
    int read_file_descriptor = 0;
    int write_file_descriptor = 0;
    int i, readFlag = 0, writeFlag = 0, argsCount = 0;
    
    
    /* Main shell loop */
    do
    {
        memset(commandLineArgs, '\0', MAX_LENGTH);

        
        printf(": ");
        fflush(stdout);     /* Flush stdout after output */
        
        getline(&commandLineArgs, &MAX, stdin);     /* User input */
        
        /* HANDLES: Comment line, blank line, 'status', and 'exit' */
        if (commandLineArgs[0] == COMMENT || strlen(commandLineArgs) == 1 ||
            strcmp(commandLineArgs, STATUS) == 0 || strcmp(commandLineArgs, EXIT) == 0)
        {
            if (strcmp(commandLineArgs, STATUS) == 0)
            {
                printf("exit value %d\n", childExitStatus);
                fflush(stdout);
            }
        }
        
        else
        {
            spawnChild = fork();    /* Initial fork call */
            
            switch (spawnChild)
            {
                /* An error with spawning the child process */
                case -1:
                    
                    fprintf(stderr, "Child spawn failed.\n");
                    free(commandLineArgs);
                    exit(2);
                    
                /* Child process spawnChild will be 0 */
                case 0:
                    
                    token = strtok_r(commandLineArgs, DELIMITER, &tokenBuffer); /* Initial call to tokenizer */
                    
                    /* Tokenize the string until there is nothing left to tokenize and store into args array */
                    while (token)
                    {
                        /* Handles special symbols <, >, & */
                        if ((!strcmp(token, "<")) || (!strcmp(token, ">")) || (!strcmp(token, "&")))
                        {
                            /* Set stdin redirect to input file */
                            if ((!strcmp(token, "<")))
                            {
                                token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                                
                                read_file_descriptor = open(token, O_RDONLY);
                                if (read_file_descriptor < 0)
                                {
                                    fprintf(stderr, "File \"%s\" could not be opened for reading.\n", token);
                                    fflush(stderr);
                                    
                                    free(commandLineArgs);
                                    exit(1);
                                }
                                
                                readFlag = 1;
                                token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                            }
                            
                            /* Set stdout redirect to output file */
                            else if ((!strcmp(token, ">")))
                            {
                                token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                                
                                write_file_descriptor = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0755);
                                if (write_file_descriptor < 0)
                                {
                                    fprintf(stderr, "File \"%s\" could not be opened for writing.\n", token);
                                    fflush(stderr);
                                    
                                    free(commandLineArgs);
                                    exit(1);
                                }
                                
                                writeFlag = 1;
                                token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                            }
                            
                            /* Handles background process (&) only if its the last string tokenized */
                            else
                            {
                                if ((!strcmp(token, BACKGROUND_PROCESS)))
                                {
                                    args[argsCount++] = token;
                                    token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                                    
                                    /* Background process has been indicated remove '&' */
                                    if (!token)
                                    {
                                        args[--argsCount] = NULL;
                                        printf("background pid is %d\n", getpid());
                                    }
                                }
                            }
                        }
                        
                        else
                        {
                            args[argsCount++] = token;
                            token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                        }
                    }
                    
                    /* Handle '$$' expansion: Process ID of the shell */
                    for (i = 0; i < argsCount; i++)
                    {
                        if ((!strcmp(args[i], PID_EXPANSION)))
                        {
                            sprintf(args[i], "%d", PPID);
                        }
                    }
                    
                    args[argsCount++] = NULL;           /* Set last value of array to NULL */
                    
                    if (readFlag)
                    {
                        dup2(read_file_descriptor, 0);  /* redirect of stdin */
                    }
                    
                    if (writeFlag)
                    {
                        dup2(write_file_descriptor, 1); /* redirect of stdout */
                    }
                    
                    free(commandLineArgs);
                    execvp(args[0], args);              /* EXEC call */
                    
                    
                    /* Will execute if exec call failed */
                    fprintf(stderr, "smallsh: %s: command not found\n", args[0]);
                    fflush(stderr);
                    
                    exit(1);
                    break;
                
                /* Parent process */
                default:
                    if (commandLineArgs[strlen(commandLineArgs) - 2] == '&')    /***********/
                    {
                        waitpid(-1, &exitMethod, WNOHANG);
                        usleep(500);
                    }
                    else
                    {
                        waitpid(spawnChild, &exitMethod, 0);  /* Block parent process until child process terminates */
                    }
                    
                    
                   
                    /* An exit status will be returned if the child process terminated successfully */
                    if (WIFEXITED(exitMethod))
                    {
                        childExitStatus = WEXITSTATUS(exitMethod);
                    }
                    
                    /* The child status terminated via signal */
                    else
                    {
                        printf("The process terminated via signal.\n");
                        fflush(stdout);
                    }
                    
                    waitpid(-1, &exitMethod, WNOHANG);
            }
            
        }
        
        
    } while (strcmp(commandLineArgs, EXIT) != 0);
    free(commandLineArgs);
    
    
    
    return 0;
}
