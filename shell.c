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

static int toggle_handler = -1;

int checkFlags(int, int);
void signalFunction(int);
void toggleFunction(int);

/****************************************************************************************/

int main(int argc, const char *argv[])
{
    pid_t spawnChild = 0;
    SIGACTION signalAction, ignoreAction, toggleAction;
    
    const int PPID = (int)getpid();
    const char DELIMITER[3] = " \n";
    
    char *token;
    char *tokenBuffer;
    char *args[MAX_ARGS];
    char *commandLineArgs = (char *)malloc(sizeof(char) * MAX_LENGTH);
    char childExitStatus[30];
    char pid_expansion[128];
    char pid_expansion_arg[128];
    char pid_expansion_temp[20];
   
    int commandLineChar;
    int count = 0;
    int wait_pid_return;
    int exitMethod;
    int check_flags_set;
    int toggle_mode = 1;
    int read_file_descriptor = 0;
    int write_file_descriptor = 0;
    int i, j, changeDirectory, backgroundFlag = 0, readFlag = 0, writeFlag = 0, argsCount = 0;
    
    
/****************************************************************************************/
    
    signalAction.sa_handler = signalFunction;                   /* Assign signal handler */
    sigfillset(&signalAction.sa_mask);                           /* Block/Delay all signals */
    signalAction.sa_flags = 0;
    
    ignoreAction.sa_handler = SIG_IGN;                          /* sig action for ignoring */
    
    toggleAction.sa_handler = toggleFunction;
    sigfillset(&toggleAction.sa_mask);
    toggleAction.sa_flags = 0;
    
    sigaction(SIGINT, &ignoreAction, NULL);                     /* SIGINT signals will be ignored for the parent */
    sigaction(SIGTSTP, &toggleAction, NULL);                    /* SIGTSTP signals will be handled in the parent */
    
    memset(childExitStatus, '\0', sizeof(childExitStatus));
    strcpy(childExitStatus, "exit value 0\n");                  /* Initial 'status' set to 0 */
    
/****************************************************************************************/
    
    /* Main shell loop */
    do
    {
        memset(commandLineArgs, '\0', MAX_LENGTH);

        printf(": ");       /* Prompt line */
        fflush(stdout);     /* Flush stdout after output */
        
        
        /* User input is read from character by character from stdin and extracted via
          getchar calls. When the end of file or a new line is encountered the loop will
          break and that indicates there is nothing else left to read from stdin. Each
          character is added to our string that will later be passed to exec if command
          is not one of the built-in commands. */
        while (1)
        {
            commandLineChar = getchar();
            if (commandLineChar == EOF) { break; }
            if (commandLineChar == '\n')
            {
                commandLineArgs[count++] = commandLineChar;
                break;
            }
            commandLineArgs[count++] = commandLineChar;
        }
        

        /* HANDLES: Comment line, blank line, 'status','exit', and 'cd' */
        if ((commandLineArgs[0] == COMMENT || strlen(commandLineArgs) == 1 ||
            strstr(commandLineArgs, "status") || strcmp(commandLineArgs, EXIT) == 0 ||
            strstr(commandLineArgs, CD)) && !strstr(commandLineArgs, "echo"))
        {
            
            /* Checks string 'status' against entered string; If located the user input
             is tokenized and checked to see if the first string token is exactly 'status';
             If true, then print exit value/signal termination of last foreground process.
             Else, treat user input as non-builtin command */
            if (strstr(commandLineArgs, "status"))
            {
                token = strtok_r(commandLineArgs, DELIMITER, &tokenBuffer);
                
                if (!strcmp(token, "status"))
                {
                    printf("%s", childExitStatus);
                    fflush(stdout);
                    
                    usleep(5000);
                }
            }
            
            /* Handles built-in 'cd' command. Looks for a substring of 'cd' initially and string tokenizes
              the string ensuring the 'cd' found is standalone and not part of a word/non built-in command */
            if (strstr(commandLineArgs, CD))
            {
                token = strtok_r(commandLineArgs, DELIMITER, &tokenBuffer);
                
                
                /* Checks to see if a valid 'cd' is found. If so, advance the string tokenizer which will
                  now contain the path specified to change directory into. If no valid 'cd' comamnd is found
                  from the while block this block will not execute */
                if (token && !strcmp(token, "cd"))
                {
                    token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                    
                    /* Handling pid expansion if there is a $$ */
                    if (token && strstr(token, PID_EXPANSION))
                    {
                        memset(pid_expansion_arg, '\0', 128);
                        memset(pid_expansion, '\0', 128);
                        strcpy(pid_expansion, token);
                        
                        for (j = 0; j < strlen(pid_expansion); j++)
                        {
                            if (pid_expansion[j] == '$' && pid_expansion[j + 1] == '$')
                            {
                                sprintf(pid_expansion_temp, "%d", PPID);
                                strcat(pid_expansion_arg, pid_expansion_temp);
                                
                                j++;
                            }
                            else
                            {
                                sprintf(pid_expansion_temp, "%c", pid_expansion[j]);
                                strcat(pid_expansion_arg, pid_expansion_temp);
                            }
                        }
                        
                        strcpy(token, pid_expansion_arg);
                    }
                    
                    /* If token is not NULL, chdir will be called on the value of token.
                     Else, 'cd' was not passed with a subsequent path indicating a chdir
                     to the home directory */
                    changeDirectory = (token) ? chdir(token) : chdir(getenv("HOME"));
                   
                    
                    /* File failed to open */
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
            
            usleep(5000);
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
                    
                    /* Signal handlers for child process */
                    sigaction(SIGINT, &signalAction, NULL);
                    sigaction(SIGTERM, &signalAction, NULL);
                    sigaction(SIGTSTP, &ignoreAction, NULL);
                    
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
                                
                                readFlag = 1;   /* Read flag gets set indicating a redirection of stdin */
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
                                
                                writeFlag = 1;  /* Write flag gets set indicating a redirection of stdout */
                                token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                            }
                            
                            /* Handles background process (&) only if its the last string tokenized */
                            else
                            {
                                /* If the token is equal to '&' and foreground only mode is disabled */
                                if ((!strcmp(token, BACKGROUND_PROCESS)) && toggle_mode)
                                {
                                    args[argsCount++] = token;
                                    
                                    
                                    /* this call to token ensures there is nothing else after '&' was encountered.
                                    If '&' is not the last token, then this will not be treated as a background
                                    process. */
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
                                
                                
                                /* Simply ignore the '&' if foreground mode is on */
                                if (!toggle_mode)
                                {
                                    token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                                }
                            }
                        }
                        
                        /* Add to string and advance token as normal if '<', '>', or '&' are not encountered */
                        else
                        {
                            args[argsCount++] = token;
                            token = strtok_r(NULL, DELIMITER, &tokenBuffer);
                        }
                    }
                    
                    
                    /* Handle '$$' expansion: Process ID of the shell */
                    memset(pid_expansion_arg, '\0', 128);
                    for (i = 0; i < argsCount; i++)
                    {
                        
                        /* The string contained $$ somewhere within it */
                        if (strstr(args[i], PID_EXPANSION))
                        {
                            memset(pid_expansion, '\0', 128);
                            strcpy(pid_expansion, args[i]);
                            
                            for (j = 0; j < strlen(pid_expansion); j++)
                            {
                                
                                /* Shell PID (parent PID) is appended string if $$ is found */
                                if (pid_expansion[j] == '$' && pid_expansion[j + 1] == '$')
                                {
                                    sprintf(pid_expansion_temp, "%d", PPID);
                                    strcat(pid_expansion_arg, pid_expansion_temp);
                                    
                                    j++;
                                }
                                
                                /* Append the character as normal */
                                else
                                {
                                    sprintf(pid_expansion_temp, "%c", pid_expansion[j]);
                                    strcat(pid_expansion_arg, pid_expansion_temp);
                                }
                            }
                            
                            strcpy(args[i], pid_expansion_arg);     /* Copy back to argument list the newly formatted string */
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
                        sigaction(SIGINT, &ignoreAction, NULL); /* Background process cant be terminated by SIGINT */
                        
                        check_flags_set = checkFlags(readFlag, writeFlag);
                        switch (check_flags_set)
                        {
                                
                            /* Write descriptor was not set with user input */
                            case 1:
                                write_file_descriptor = open("/dev/null", O_WRONLY);
                                dup2(write_file_descriptor, 1);
                                break;
                                
                            /* Read descriptor was not set with user input */
                            case 2:
                                read_file_descriptor = open("/dev/null", O_RDONLY);
                                dup2(read_file_descriptor, 0);
                                break;
                                
                            /* Both read and write descriptors were set by user input */
                            case 3:
                                break;
                                
                            /* Neither descriptor was set by user input */
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
                    
                    /* Signals handled for parent process */
                    sigaction(SIGINT, &ignoreAction, NULL);
                    sigaction(SIGTERM, &ignoreAction, NULL);
                    
                    
                    /* Handles background processes with continuing execution with WNOHANG. Will not execute
                     if in foreground only mode */
                    if (commandLineArgs[strlen(commandLineArgs) - 2] == '&' && toggle_mode)    /***********/
                    {
                        wait_pid_return = waitpid(-1, &exitMethod, WNOHANG);
                        if (wait_pid_return != -1 && wait_pid_return != 0)
                        {
                            if (WIFEXITED(exitMethod))
                            {
                                printf("background pid %d is done: exit value %d\n", wait_pid_return, WEXITSTATUS(exitMethod));
                                fflush(stdout);
                            }
                            else
                            {
                                printf("background pid %d is done: terminated by signal %d\n", wait_pid_return, WTERMSIG(exitMethod));
                                fflush(stdout);
                            }
                            
                        }
                        usleep(5000);
                    }
                    else
                    {
                        
                        /* Handles foreground processes waiting for foreground process to terminated before
                         resuming execution in the parent process */
                        waitpid(spawnChild, &exitMethod, 0);            /* Block parent process until child process terminates */
                        waitpid(spawnChild, &exitMethod, 0);
                        
                        /* An exit status will be returned if the child process terminated successfully */
                        if (WIFEXITED(exitMethod))
                        {
                            memset(childExitStatus, '\0', sizeof(childExitStatus));
                            sprintf(childExitStatus, "exit value %d\n", WEXITSTATUS(exitMethod));
                        }
                        
                        /* The child status terminated via signal */
                        else
                        {
                            if (WTERMSIG(exitMethod) != 127 && WTERMSIG(exitMethod) != 11)
                            {
                                if (WTERMSIG(exitMethod) == 0) {}   /* Catches a SIGTSTP pressed during a foreground process execution */
                                
                                memset(childExitStatus, '\0', sizeof(childExitStatus));
                                sprintf(childExitStatus, "terminated by signal %d\n", WTERMSIG(exitMethod));
                                printf("%s", childExitStatus);
                                fflush(stdout);
                            }
                        }
                    }
                    
            }
            
        }
        
        /* Will be set to 5 if a SIGTSTP signal was sent. Handles the toggling between
          foreground only mode and normal mode */
        if (toggle_handler == 5)
        {
            if (toggle_mode)
            {
                printf("\nEntering foreground-only mode (& is now ignored)\n");
                fflush(stdout);
                toggle_mode = 0;
            }
            else
            {
                printf("\nExiting foreground-only mode\n");
                fflush(stdout);
                toggle_mode = 1;
            }
        }
        
        /* Clean up any zombie processes that terminated via successful termination and signals */
        for (i = 0; i < 5; i++)
        {
            wait_pid_return = waitpid(-1, &exitMethod, WNOHANG);
            if (wait_pid_return != -1 && wait_pid_return != 0)
            {
                if (WIFEXITED(exitMethod))
                {
                    printf("background pid %d is done: exit value %d\n", wait_pid_return, WEXITSTATUS(exitMethod));
                    fflush(stdout);
                }
                else
                {
                    printf("background pid %d is done: terminated by signal %d\n", wait_pid_return, WTERMSIG(exitMethod));
                    fflush(stdout);
                }
                
            }
        }
        
        toggle_handler = -1;
        count = 0;
    } while (strcmp(commandLineArgs, EXIT) != 0); /* Input entered was 'exit' */
    free(commandLineArgs);
    
    
    
    return 0;
}

/****************************************************************************************/

/* File redirection check helper */
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

/* Signal handling functions */
void signalFunction(int signal){}

void toggleFunction(int signal)
{
    toggle_handler = 5;
}
