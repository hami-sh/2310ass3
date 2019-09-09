#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>

int main(int argc, char** argv)
{
    int fd1[2]; //hold both ends of pipe
    int fd2[2];
    int x[1];
    x[0] = 0;

    if (pipe(fd1)==-1) //create pipe
    {
        fprintf(stderr, "Pipe Failed" );
        return 1;
    }
    if (pipe(fd2)==-1)
    {
        fprintf(stderr, "Pipe Failed" );
        return 1;
    }

    pid_t p = fork();
    if (p < 0) {
        printf("fork failed\n");
    }
    if (p) {
        // parent
        int new[1];
        close(fd1[0]);  // Close reading end of first pipe

        // Write number and close writing end of first pipe.
        write(fd1[1], x, 1);
        close(fd1[1]);

        // Wait for child to send back.
        wait(NULL);

        close(fd2[1]); // Close writing end of second pipe

        // Read string from child, print it and close
        // reading end.
        read(fd2[0], new, 1);
        printf("Parent read %d\n", *new);
        close(fd2[0]);

    } else {
        // child
        close(fd1[1]);  // Close writing end of first pipe

        // Read an int using first pipe
        char concat_str[100];
        int found[1];
        read(fd1[0], found, 1);

        // increase by 1
        found[0] += 1;


        // Close both reading ends
        close(fd1[0]);
        close(fd2[0]);

        // Write concatenated string and close writing end
        write(fd2[1], found, 1);
        close(fd2[1]);

        exit(0);
    }

}