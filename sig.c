#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

bool itHappened = false;	// global variable

void notice(int s) {	// parameter for if we handle multiple signals
    itHappened = true;
}

int main(int argc, char** argv) {
    struct sigaction sa;
    sa.sa_handler = notice;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, 0);
    while (true) {
        while (!itHappened) {
            struct timespec nap;
            nap.tv_sec = 0;
            nap.tv_nsec = 5000000000;
            nanosleep(&nap, 0);	// you don't need to know this call
        }
        printf("It happened\n");
        itHappened = false;
    }
    return 0;
}