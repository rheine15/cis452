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

//path variables for ftok
char *path = "/tmp";
int id = 'C';

//globals for shm id and pointer
int shmId;
struct readwrite* shared_mem;

//struct containing flags and data
struct readwrite{
  int read;		//reader1 sets to '1' when completed reading
  int read2;		//reader2 sets to '1' when completed reading
  char data [BUFSIZE];
};

int main(int argc, char *argv[])
{
  //exit if a '1' or '2' was not provided
  if(argc < 2){
      printf("Please provide a '1' or '2' to identify which reader you are\n");
      exit(0);
  }
  if(*argv[1] != '1' && *argv[1] != '2') {
      printf("Please provide a '1' or '2' to identify which reader you are\n");
      exit(0);
  }
  
  //struct for shared mem stats
  struct shmid_ds shared_mem_stats;
  //obtain passkey for shared memory
  key_t passkey;
  passkey = ftok(path,id);
  
  //register signal handler for exiting program
  signal(SIGINT, exitHandler);
  
  //setup shared memory space
  if ((shmId = shmget(passkey, sizeof(struct readwrite), S_IRUSR|S_IWUSR)) < 0) {
    perror("Please start writer before reader\n");
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
  printf("I am reader %d\n", *argv[1]-'1'+1);
   
  //stay alive to continuously read data
  while(1){  
      if(*argv[1] == '1' && shared_mem->read){
	printf("Read from shared memory: %s\n",shared_mem->data);
	shared_mem->read = 0;
      } else if (*argv[1] == '2' && shared_mem->read2){
	printf("Read from shared memory: %s\n",shared_mem->data);
	shared_mem->read2 = 0;
      }
  }
  
  return 0;
}

void exitHandler (int sigNum)
{
  printf(" received.\n"); 
  
  //detach from shared memory
  if(shmdt (shared_mem) < 0){
    perror("just can't let go\n");
    exit (1);
  }
  //exit process
  exit(0);
}