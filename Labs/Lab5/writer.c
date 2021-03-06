#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define BUFSIZE 4096
void exitHandler (int);

//struct containing flags and data
struct readwrite{
  int read;		//reader1 sets to '1' when completed reading
  int read2;		//reader2 sets to '1' when completed reading
  char data [BUFSIZE];
};

//path variables for ftok
char *path = "/tmp";
int id = 'C';

//globals for shm id and pointer
int shmId;
struct readwrite* shared_mem;

int main ()
{
  //struct for shared mem stats
  struct shmid_ds shared_mem_stats;
  //obtain passkey for shared memory
  key_t passkey;
  passkey = ftok(path,id);
  
  //register signal handler for exiting program
  signal(SIGINT, exitHandler);
  
  //setup shared memory space
  if ((shmId = shmget(passkey, sizeof(struct readwrite), IPC_CREAT|S_IRUSR|S_IWUSR)) < 0) {
    perror("i can't get no..\n");
    exit(1);
  }
  if ((shared_mem = shmat (shmId, 0, 0)) == (void*) -1) { //attach
    perror("can't attach\n");
    exit(1);
  }  
  if (shmctl(shmId, IPC_STAT, &shared_mem_stats) < 0) { //get stats
    perror("can't get stats\n");
    exit(1);
  }
  
  printf("shmId: %d\n", shmId);
  printf("Shared memory size: %zd bytes\n", shared_mem_stats.shm_segsz);
  printf("I am the writer\n");
   printf("Provide input:\n");
  
  //stay alive to continuously send data
  while(1){
    //check if both readers are done
    if(!shared_mem->read && !shared_mem->read2){     
      fgets(shared_mem->data, BUFSIZE, stdin);  
      //chop off newline character
      shared_mem->data[strlen(shared_mem->data)-1]='\0';

      //set flags for readers
      shared_mem->read = 1;
      shared_mem->read2 = 1;
    }
  }
  
  return 0;
}

void exitHandler (int sigNum)
{
  printf(" received.\n");
  
  //detach and deallocate from shared memory
  if(shmdt (shared_mem) < 0) {
    perror("just can't let go\n");
    exit (1);
  }
  if(shmctl(shmId, IPC_RMID, 0) < 0) {
    perror("can't deallocate\n");
    exit(1);
  }
  //exit process
  exit(0);
}