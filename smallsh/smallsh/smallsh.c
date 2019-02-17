#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LENGTH              2048
#define MAX_ARGS                512
#define COMMENT                 '#'
#define PID_EXPANSION           "$$"
#define BACKGROUND_PROCESS      "&"
#define STATUS                  "status\n"
#define EXIT                    "exit\n"
#define CD                      "cd"

typedef struct sigaction SIGACTION;

/****************************************************************************************/

int checkFlags(int, int);
void signalFunction(int);

/****************************************************************************************/

int main(int argc, const char *argv[])
{
    pid_t spawnChild = 0;
    size_t MAX = MAX_LENGTH;
    SIGACTION signalAction, ignoreAction;
    
    
    const int PPID = (int)getpid();
    const char DELIMITER[3] = " \n";
    
    char *token;
    char *tokenBuffer;
    char *args[MAX_ARGS];
    char *commandLineArgs = (char *)malloc(sizeof(char) * MAX_LENGTH);
    char childExitStatus[30];
   
    int wait_pid_return;
    int exitMethod;
    int check_flags_set;
    int read_file_descriptor = 0;
    int write_file_descriptor = 0;
    int i, changeDirectory, pid, backgroundFlag = 0, readFlag = 0, writeFlag = 0, argsCount = 0;
    
    
/****************************************************************************************/
    
    signalAction.sa_handler = signalFunction;
    sigfillset(&signalAction.sa_mask);
    signalAction.sa_flags = 0;
    
    ignoreAction.sa_handler = SIG_IGN;
    
    
    memset(childExitStatus, '\0', sizeof(childExitStatus));
    strcpy(childExitStatus, "exit value 0");
    /* Main shell loop */
    do
    {
        memset(commandLineArgs, '\0', MAX_LENGTH);

        
        printf(": ");
        fflush(stdout);     /* Flush stdout after output */
        
        getline(&commandLineArgs, &MAX, stdin);     /* User input */
        
        /* HANDLES: Comment line, blank line, 'status','exit', and 'cd' */
        if (commandLineArgs[0] == COMMENT || strlen(commandLineArgs) == 1 ||
            strcmp(commandLineArgs, STATUS) == 0 || strcmp(commandLineArgs, EXIT) == 0 ||
            strstr(commandLineArgs, CD))
        {
            if (strcmp(commandLineArgs, STATUS) == 0)
            {
                printf("%s", childExitStatus);
                fflush(stdout);
            }
            
            /* Handles built-in 'cd' command. Looks for a substring of 'cd' initially and string tokenizes
              the string ensuring the 'cd' found is standalone and not part of a word/non built-in command */
            if (strstr(commandLineArgs, CD))
            {
                token = strtok_r(commandLineArgs, DELIMITER, &tokenBuffer);
                
                /* Looks for a standalone 'cd' command. If none is found then the while will break
                  when token returns NULL */
                while (token)
                {
                    if (!strcmp(token, "cd")) { break; }
                    token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                }
                
                /* Checks to see if a valid 'cd' is found. If so, advance the string tokenizer which will
                  now contain the path specified to change directory into. If no valid 'cd' comamnd is found
                  from the while block this block will not execute */
                if (token && !strcmp(token, "cd"))
                {
                    token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                    
                    changeDirectory = (token) ? chdir(token) : chdir(getenv("HOME"));
                    printf("%s\n", getenv("HOME"));
                    if (changeDirectory < 0)
                    {
                        printf("No such file or directory: %s\n", token);
                        fflush(stdout);
                    }
                }
                else
                {
                    printf("smallsh: command not found\n");
                    fflush(stdout);
                }
                
                token = NULL;
                tokenBuffer = NULL;
            }
        }
        
        else
        {
            spawnChild = fork();    /* Initial fork call */
//            sigaction(SIGINT, &signalAction, NULL);
            
            
            switch (spawnChild)
            {
                /* An error with spawning the child process */
                case -1:
                    
                    fprintf(stderr, "Child spawn failed.\n");
                    free(commandLineArgs);
                    exit(2);
                    
                /* Child process spawnChild will be 0 */
                case 0:
                    sigaction(SIGINT, &signalAction, NULL);
                    sigaction(SIGTERM, &signalAction, NULL);
                    
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
                                        fflush(stdout);
                                        
                                        backgroundFlag = 1; /* Set background flag */
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
                    
                    
                    /* The process will be a background process check for any specified redirections */
                    if (backgroundFlag)
                    {
                        check_flags_set = checkFlags(readFlag, writeFlag);
                        switch (check_flags_set)
                        {
                            case 1:
                                write_file_descriptor = open("/dev/null", O_WRONLY);
                                dup2(write_file_descriptor, 1);
                                break;
                                
                            case 2:
                                read_file_descriptor = open("/dev/null", O_RDONLY);
                                dup2(read_file_descriptor, 0);
                                break;
                                
                            case 3:
                                break;
                                
                            default:
                                write_file_descriptor = open("/dev/null", O_WRONLY);
                                read_file_descriptor = open("/dev/null", O_RDONLY);
                                
                                dup2(write_file_descriptor, 1);
                                dup2(read_file_descriptor, 0);
                        }
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
                    sigaction(SIGINT, &ignoreAction, NULL);
                    sigaction(SIGTERM, &ignoreAction, NULL);
                    
                    if (commandLineArgs[strlen(commandLineArgs) - 2] == '&')    /***********/
                    {
                        waitpid(-1, &exitMethod, WNOHANG);
                        usleep(5000);
                    }
                    else
                    {
                        waitpid(spawnChild, &exitMethod, 0);  /* Block parent process until child process terminates */
                    }
                    
                    
                   
                    /* An exit status will be returned if the child process terminated successfully */
                    if (WIFEXITED(exitMethod))
                    {
                        memset(childExitStatus, '\0', sizeof(childExitStatus));
                        sprintf(childExitStatus, "exit value %d\n", WEXITSTATUS(exitMethod));
                    }
                    
                    /* The child status terminated via signal */
                    else
                    {
                        if (WTERMSIG(exitMethod) != 127)
                        {
                            memset(childExitStatus, '\0', sizeof(childExitStatus));
                            sprintf(childExitStatus, "terminated by signal %d\n", WTERMSIG(exitMethod));
                            printf("%s", childExitStatus);
                            fflush(stdout);
                        }
                    }
                    
//                    wait_pid_return = waitpid(-1, &exitMethod, WNOHANG);
//                    if (wait_pid_return != -1 && wait_pid_return != 0)
//                    {
//                        if (WIFEXITED(exitMethod))
//                        {
//                            printf("background pid %d is done: exit value %d\n", wait_pid_return, WEXITSTATUS(exitMethod));
//                        }
//                        else
//                        {
//                            printf("background pid %d is done: terminated by signal %d\n", wait_pid_return, WTERMSIG(exitMethod));
//                        }
//
//                    }
            }
            
        }
        
        wait_pid_return = waitpid(-1, &exitMethod, WNOHANG);
        if (wait_pid_return != -1 && wait_pid_return != 0)
        {
            if (WIFEXITED(exitMethod))
            {
                printf("background pid %d is done: exit value %d\n", wait_pid_return, WEXITSTATUS(exitMethod));
            }
            else
            {
                printf("background pid %d is done: terminated by signal %d\n", wait_pid_return, WTERMSIG(exitMethod));
            }
            
        }
        
        
    } while (strcmp(commandLineArgs, EXIT) != 0);
    free(commandLineArgs);
    
    
    
    return 0;
}

/****************************************************************************************/

int checkFlags(int readFlag, int writeFlag)
{
    if (readFlag && writeFlag)          /* Both redirection flags were set */
    {
        return 3;
    }
    else if (!readFlag && writeFlag)    /* stdout redirection was set but not stdin */
    {
        return 2;
    }
    else if (readFlag && !writeFlag)    /* stdin redirection was set but not stdout */
    {
        return 1;
    }
    
    return 0;                           /* Neither one of the redirection flags were set */
}

void signalFunction(int signal)
{
    
}
