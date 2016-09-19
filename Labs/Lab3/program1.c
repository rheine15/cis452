#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void sigHandler (int);

int main()
{
   // signal (SIGINT, sigHandler);
    printf ("waiting...\n");
    //pause();
    while(1){}
    return 0;
}

void
sigHandler (int sigNum)
{
    printf (" received an interrupt.\n");
    // this is where shutdown code would be inserted
   // sleep (1);
   // printf ("outta here.\n");
   // exit(0);
} 
