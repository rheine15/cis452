#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int WORDSIZE = 256;

void* find_file (void* arg);
void exitHandler (int);

//global variable for request count
int file_request_count = 0;

//extra credit variables for service count and total access time
int file_service_count = 0;
int total_access_time = 0;
pthread_mutex_t lock;

int main()
{ 
  //status for error values
  int status;
  
  //initial prompt to user
  printf("Enter file names to simulate retreival. Use Ctrl+C to exit:\n");
  //register signal handler for exiting program
  signal(SIGINT, exitHandler);
  //seed the random number generator
  srand(time(NULL));
  
  //initialize the mutex
  if((status = pthread_mutex_init(&lock, NULL)) !=0){
     fprintf (stderr, "---Mutex init error %d: %s\n", status, strerror(status));
     exit(1);
  }
  
  while(1){
    //setup variables for each worker thread
    char file_name[WORDSIZE];
    pthread_t worker;
       
    //get file name from user
    fgets(file_name, WORDSIZE, stdin);
    //chop off newline character
    file_name[strlen(file_name)-1]='\0';
    //increment request count
    file_request_count++;
    
    // create and start a thread executing the "find_file" function
    // pass each thread a pointer to the file name so it can later make its own copy
    if ((status = pthread_create (&worker, NULL,  find_file, file_name)) != 0) {
      fprintf (stderr, "---Thread create error %d: %s\n", status, strerror(status));
      exit (1);
    }
    
    // detach from the worker thread in order to receive another request
    if ((status = pthread_detach (worker)) != 0) {
      fprintf (stderr, "---Detach error %d: %s\n", status, strerror(status));
      exit (1);
    }
  }
  
  return 0;
}

void* find_file (void* arg)
{
  //TODO: race condition if file_name is changed by a second 
  //TODO: request before first copy is completed
  // casting to string and copying to local variable
  char file_request[WORDSIZE];
  strcpy(file_request,(char*) arg);
  
  // print out the file name requested
  printf ("-Worker retreiving '%s'\n", file_request);
  
  int r,t = 0; 
  r = rand()%5; //random number between 0 and 44
  if(r){ //80% chance of being 1 thru 4
   sleep(1);
   //fflush(stdin);
   printf ("-File '%s' found in cache\n", file_request);
  } else { //20% chance of being 0
    t = (rand()%4) + 7; //random number between 7 and 10
    sleep(t);
    //TODO: Do we need to block or is this simulating it???
    printf ("-File '%s' found in hard drive after %d seconds\n", file_request, t);
  }
  
  //increment service count and access time. Make sure to lock
  pthread_mutex_lock(&lock);
  file_service_count++;
  if(r){ //was found in cache
   total_access_time++; 
  } else{ //was not found in cache
   total_access_time += t;
  }
  pthread_mutex_unlock(&lock);
  
  //TODO: Is pthread_exit or return(NULL) better???
  pthread_exit(NULL);
}

void exitHandler (int sigNum)
{
  printf(" received.\n");
  printf("Files Requested: %d\n",file_request_count);
  printf("Files Serviced: %d\n",file_service_count);
  printf("Total Service Time: %d seconds\n",total_access_time);
  printf("Average Service Time: %d seconds\n",total_access_time/file_service_count);
  exit(0);
}

