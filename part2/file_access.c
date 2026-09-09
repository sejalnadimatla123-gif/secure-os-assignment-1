#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define FILE_NAME "part2/secret.txt"

void test_read(void)
{
    int fd = open(FILE_NAME, O_RDONLY);

    if (fd == -1)
    {
        perror("Read access");
        printf("READ: FAILED\n");
    }
    else
    {
        printf("READ: SUCCESS\n");
        close(fd);
    }
}

void test_write(void)
{
    int fd = open(FILE_NAME, O_WRONLY);

    if (fd == -1)
    {
        perror("Write access");
        printf("WRITE: FAILED\n");
    }
    else
    {
        printf("WRITE: SUCCESS\n");
        close(fd);
    }
}

int main(void)
{
    printf("Testing file: %s\n\n", FILE_NAME);

    test_read();
    test_write();

    return 0;
}