#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LENGTH      2048
#define COMMENT         '#'
#define EXIT            "exit\n"


int main(int argc, const char *argv[])
{
    pid_t spawnChild = 0;
    size_t MAX = MAX_LENGTH;
    
    char *commandLineArgs = (char *)malloc(sizeof(char) * MAX_LENGTH);
    
    int execReturn;
    int childExitStatus;
    int exitMethod;
    
    
    /* Main shell loop */
    do
    {
        memset(commandLineArgs, '\0', MAX_LENGTH);
        
        printf(": ");
        fflush(stdout);     /* Flush stdout after output */
        
        getline(&commandLineArgs, &MAX, stdin);     /* User input */
        
        /* Handles comment line or blank line */
        if (commandLineArgs[0] == COMMENT || strlen(commandLineArgs) == 1) {}
        
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
                    printf("Child spawned.\n");
                    
                    execReturn = execlp("ls", "ls", "test", NULL);
                    if (execReturn == -1)
                    {
                        fprintf(stderr, "smallsh: %s: command not found\n", "lr");
                        fflush(stderr);
                    }
                    
                    exit(1);
                    break;
                
                /* Parent process */
                default:
                    waitpid(spawnChild, &exitMethod, 0);  /* Block parent process until child process terminates */
                    printf("%d\n", exitMethod);
                    /* An exit status will be returned if the child process terminated successfully */
                    if (WIFEXITED(exitMethod))
                    {
                        childExitStatus = WEXITSTATUS(exitMethod);
                        printf("The exit status is: %d\n", childExitStatus);
                        fflush(stdout);
                    }
                    
                    /* The child status terminated via signal */
                    else
                    {
                        printf("The process terminated via signal.\n");
                        fflush(stdout);
                    }
            }
            
        }
        
        
    } while (strcmp(commandLineArgs, EXIT) != 0);
    free(commandLineArgs);
    
    
    
    return 0;
}
