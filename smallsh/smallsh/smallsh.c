#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_LENGTH      2048
#define MAX_ARGS        512
#define COMMENT         '#'
#define EXIT            "exit\n"


int main(int argc, const char *argv[])
{
    pid_t spawnChild = 0;
    size_t MAX = MAX_LENGTH;
    
    
    const char delimiter[3] = " \n";
    
    char *token;
    char *tokenBuffer;
    char *args[MAX_ARGS];
    char *commandLineArgs = (char *)malloc(sizeof(char) * MAX_LENGTH);
    
    int childExitStatus;
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
                    
                    token = strtok_r(commandLineArgs, delimiter, &tokenBuffer); /* Initial call to tokenizer */
                    
                    /* Tokenize the string until there is nothing left to tokenize and store into args array */
                    while (token)
                    {
                        /* Handles special symbols <, >, & */
                        if ((!strcmp(token, "<")) || (!strcmp(token, ">")) || (!strcmp(token, "&")))
                        {
                            /* Set stdin redirect to input file */
                            if ((!strcmp(token, "<")))
                            {
                                token = strtok_r(NULL, delimiter, &tokenBuffer);
                                
                                args[argsCount++] = token;
                                read_file_descriptor = open(token, O_RDONLY);
                                if (read_file_descriptor < 0)
                                {
                                    fprintf(stderr, "File \"%s\" could not be opened for reading.\n", token);
                                    fflush(stderr);
                                    exit(1);
                                }
                                
                                readFlag = 1;
                                token = strtok_r(NULL, delimiter, &tokenBuffer);
                            }
                            
                            /* Set stdout redirect to output file */
                            else if ((!strcmp(token, ">")))
                            {
                                token = strtok_r(NULL, delimiter, &tokenBuffer);
                                
                                args[argsCount++] = token;
                                write_file_descriptor = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0755);
                                if (write_file_descriptor < 0)
                                {
                                    fprintf(stderr, "File \"%s\" could not be opened for writing.\n", token);
                                    fflush(stderr);
                                    exit(1);
                                }
                                
                                writeFlag = 1;
                                token = strtok_r(NULL, delimiter, &tokenBuffer);
                            }
                            
                            else
                            {
                                
                            }
                            
                            
                        }
                        else
                        {
                            args[argsCount++] = token;
                            token = strtok_r(NULL, delimiter, &tokenBuffer);
                        }
                    }
                    
                    for (i = 0; i < argsCount; i++)
                    {
                        printf("%s ", args[i]);
                    }
                    printf("\n");
                    fflush(stdout);
                    
                    args[argsCount++] = NULL;   /* Set last value of array to NULL */
                    
                    if (readFlag)
                    {
                        dup2(read_file_descriptor, 0);  /* redirect of stdin */
                    }
                    
                    if (writeFlag)
                    {
                        dup2(write_file_descriptor, 1); /* redirect of stdout */
                    }
                    
                    
                    execvp(args[0], args);
                    
                    
                    /* Will execute if exec call failed */
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
