#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    {
        printf
        (
            "Process-ID: %i\n"
            "Parent Process-ID: %i\n"
            "User-ID: %u\n"
            "Group-ID: %u\n",
            getpid(), getppid(), getuid(), getgid()
        );
    }

    {
        #define GETCWD_LIMIT 1024
        char workingDirectory[GETCWD_LIMIT];

        if (getcwd(workingDirectory, GETCWD_LIMIT) == NULL) {
            printf("--ERROR RETRIEVING WORKING DIRECTORY--");
            return -1;
        }
        else workingDirectory[GETCWD_LIMIT] = '\0';

        printf("Current Working Directory: \"%s\"", workingDirectory);
    }

    return 0;
}