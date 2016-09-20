#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

void sigHandler (int);

int main()
{
  pid_t pid;
  
  //Register our two user signals to the sigHandler
  signal(SIGUSR1, sigHandler);
  signal(SIGUSR2, sigHandler);
  
  pid = fork();
  
  if(pid < 0){ //fork failed
    perror("fork failed");
    exit(1);
  } else if(pid > 0){ //I am the parent  
    //Register the ^C to the sigHandler for exitting
    signal(SIGINT, sigHandler);
    
    printf("spawned child PID# %d\n",pid);
    
    printf("waiting...\t");
    fflush(stdout);
    pause();
    //TODO: why is this not returning? http://stackoverflow.com/questions/16041754/how-to-use-sigsuspend
    printf("unpaused!\n");
    
  } else if(pid == 0) { //I am a child
    //stay alive to send signals
    int rTime,rSig;
    while(1){
      srand(time(NULL));
      //random time between 1 and 5
      rTime = (rand()%5)+1;
      //random signal 1 or 2
      rSig = rand()%2;
      
      sleep(rTime);
      if(rSig){
	if (kill(getpid(),SIGUSR1) < 0){
	  perror("kill failed");
	  exit(1);
	}
      } else {
	if (kill(getpid(),SIGUSR2) < 0){
	  perror("kill failed");
	  exit(1);
	}
      }    
    } //end while
  }
  return 0;
}

void sigHandler (int sigNum)
{
  if(sigNum == SIGINT){
    printf (" received. That's it. I'm shutting you down...\n");
    exit(0);
  } else if (sigNum == SIGUSR1){
    printf ("received a SIGUSR1 signal\n");
  } else if (sigNum == SIGUSR2){
    printf ("received a SIGUSR2 signal\n");
  }
  
  printf("waiting...\t");
  fflush(stdout);
} 
