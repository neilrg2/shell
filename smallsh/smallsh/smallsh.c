#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LENGTH      2048
#define COMMENT         '#'
#define EXIT            "exit\n"


int main(int argc, const char *argv[])
{
    pid_t spawnChild = 0;
    size_t MAX = MAX_LENGTH;
    
    
    const char delimiter[3] = " \n";
    
    char *commandLineArgs = (char *)malloc(sizeof(char) * MAX_LENGTH);
    
    int execReturn;
    int childExitStatus;
    int exitMethod;
    int file_descriptor;
    
    
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
                    
                    file_descriptor = open("test", O_WRONLY | O_CREAT | O_TRUNC, 0755);
                    
                    /* Redirection of stdout to open file descriptor */
                    dup2(file_descriptor, 1);
                    execlp("ls", "ls", NULL);
                    
                    
                    
                    
                    fprintf(stderr, "smallsh: %s: command not found\n", "lr");
                    fflush(stderr);
                    
                    exit(1);
                    break;
                
                /* Parent process */
                default:
                    waitpid(spawnChild, &exitMethod, 0);  /* Block parent process until child process terminates */
                   
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
            }
            
        }
        
        
    } while (strcmp(commandLineArgs, EXIT) != 0);
    free(commandLineArgs);
    
    
    
    return 0;
}
