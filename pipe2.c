#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid;
    int r;
    char buf[1024];
    char cp[50];
    char ans;
    int readpipe[2];
    int writepipe[2];
    long int a;
    int b;
    a=pipe(readpipe);
    b=pipe(writepipe);

    if (a == -1) { perror("pipe"); exit(EXIT_FAILURE); }
    if (b == -1) { perror("pipe"); exit(EXIT_FAILURE); }
    printf("\nSIZE OF CP IS %d",sizeof(cp));
    printf("\nSEND SOMETHING TO CHILD PROCESS\t");
    scanf("%[^\n]%*c",&ans);
    fflush(stdin);
    pid=fork();
    if(pid==-1)
    {
        printf("pid:main");
        exit(1);
    }
    while(ans=='y' || ans=='Y')
    {
        if(pid==0)
        {
            //CHILD PROCESS
            if(ans=='n')
            {
                kill(pid,SIGKILL);
            }

            close(readpipe[1]);
            close(writepipe[0]);
            if(read(readpipe[0],buf,sizeof(buf)) < 0)
            {
                printf("wait 1\n");
                break;
            }
            printf("\nChild Process Read: %s\n",buf);
            printf("\n(child)Enter data:\t");
            fflush(stdin);
            fgets(cp, 50, stdin);
            printf("\nData Written to Parent %s",cp);
            if(write(writepipe[1],cp,strlen(cp)+1) < 0)
            {
                printf("wait 2\n");
                break;
            }

        }
        else
        {
            close(readpipe[0]);
            close(writepipe[1]);
            printf("\n(Parent)Enter data\t");
            fgets(cp, 50, stdin);
            printf("\nData Written to Child: %s",cp);
            if(write(readpipe[1],cp,strlen(cp)+1) < 0)
            {
                printf("wait 3\n");
                break;
            }

            if(read(writepipe[0],buf,sizeof(buf)) < 0)
            {
                printf("wait 4\n");
                break;
            }
            printf("\nParent Process Read: %s\n",buf);
            printf("\nSEND SOMETHING TO CHILD PROCESS\t");
            fflush(stdin);
            scanf(" %[^\n]%*c",&ans);
            if(ans=='n')
            {
                kill(pid,SIGKILL);
            }
        }
    }
    close(readpipe[1]);
    close(writepipe[0]);
    close(readpipe[0]);
    close(writepipe[1]);
    return 0;
}