#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LENGTH      2048
#define COMMENT         '#'
#define EXIT            "exit\n"


int main(int argc, const char * argv[])
{
//    pid_t spawn;
    size_t MAX = MAX_LENGTH;
    char *commandLineArgs = (char *)malloc(sizeof(char) * MAX_LENGTH);
    
    
    
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
            
        }
        
        
        
    } while (strcmp(commandLineArgs, EXIT) != 0);
    free(commandLineArgs);
    
    
    
    
    
    
//    spawn = fork();
//    switch (spawn)
//    {
//        case -1:
//            fprintf(stderr, "Forked process failed.\n");
//            exit(1);
//
//        case 0:
//            printf("THIS IS THE CHILD PROCESS.\n");
//            printf("CHILD PROCESS ID: %d\n", getpid());
//            exit(0);
//            break;
//
//        default:
//            printf("THIS IS THE PARENT PROCESS.\n");
//            printf("PARENT PROCESS ID: %d\n", getpid());
//    }
//
//
//    wait(&spawn);
//    printf("SPAWN: %d\n", spawn);
    return 0;
}
