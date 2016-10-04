#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define READ 0
#define WRITE 1
#define MAX 1024

struct record{
  int transactionNum;
  int customerNum;
  double amount;
};

void sendNextSig (int);
void readReadySig1 (int);
void readReadySig2 (int);
struct record* sort(char*,int*);
void printStructArray (struct record*,int);

//global variables for sigHandler
int sendNext = 1;
int readReady1 = 0;
int readReady2 = 0;

int main(int argc, char* argv[])
{
  pid_t pid1, pid2;
  int fd1[2], fd2[2];
  ssize_t num1, num2; 
  
  //TODO: stay alive to fork multiple times
  //while(1){
  
  //set up the two pipes
  if (pipe(fd1) < 0) {
    perror ("plumbing problem");
    exit(1);
  }
  if (pipe(fd2) < 0) {
    perror ("plumbing problem");
    exit(1);
  }
  
  pid1 = fork();
  
  if(pid1 < 0){
    perror("fork failed");
    exit(1);
  } else if(pid1 > 0){ //I am the parent
    
    pid2 = fork();
    
    if(pid2 < 0){
      perror("fork failed");
      exit(1);
    } else if(pid2 > 0){ //I am the parent (merger)
      printf("I am the merger: %ld\n",(long)getpid());
      //register signal handler. Will be sent from child to let know have loaded another input.
      signal(SIGUSR1, readReadySig1);
      printf("Registered %ld on: %d\n",(long)pid1,SIGUSR1);
      printf("Registered %ld on: %d\n",(long)pid2,SIGUSR2);
      signal(SIGUSR2, readReadySig2);
      
      //close unneccesary file descriptors
      close (fd1[WRITE]);
      close (fd2[WRITE]);
      
      //keep track of which pipe to read from
      int lastRead = 0;
      char str1[MAX],str2[MAX],copy1[MAX],copy2[MAX];
      char *tempTok1, *tempTok2;
      int custNum1, custNum2;
      
      //stay alive
      while(1) {
	if(readReady1 && readReady2){
	  //printf("Beginning read\n");
	  //read a single record from the pipe
	  if(lastRead != 2){
	    num1 = read(fd1[READ], (void *) str1, (size_t)  sizeof (str1));
	  }
	  if(lastRead != 1){
	    num2 = read(fd2[READ], (void *) str2, (size_t)  sizeof (str2));
	  }
	  //check if there was an error
	  if (num1 > MAX || num2 > MAX) {
	    perror ("pipe read error\n");
	    exit(1);
	  }
	  //copy strings because strtok destroys input
	  strcpy(copy1,str1);
	  strcpy(copy2,str2);
	  //get to 2nd string
	  tempTok1 = strtok(copy1," ");
	  tempTok1 = strtok(NULL," ");
	  tempTok2 = strtok(copy2," ");
	  tempTok2 = strtok(NULL," ");	  
	  //convert to integers
	  custNum1 = atoi(tempTok1);
	  custNum2 = atoi(tempTok2);
	  
	  if(custNum1 < custNum2){
	    printf(str1);
	    readReady1 = 0;
	    lastRead = 1;
	    //printf("ready 1 off\n");
	    //send signal when done reading to request another record.
	    if (kill(pid1,SIGUSR1) < 0){
	      perror("kill failed");
	      exit(1);
	    }
	    //printf("Requesting from child %ld\n", (long)pid1);  
	  } else {
	    printf(str2);
	    readReady2 = 0;
	    lastRead = 2;
	    //printf("ready 2 off\n");
	    //send signal when done reading to request another record.
	    if (kill(pid2,SIGUSR1) < 0){
	      perror("kill failed");
	      exit(1);
	    }
	    //printf("Requesting from child %ld\n", (long)pid2);
	  }
	  //TODO: exit when done sorting
	} //end of read
      }
    } else if(pid2 == 0){ //I am child 2
      printf("I am child %ld, Parent is %ld\n", (long)getpid(), (long)getppid());
      
      //TODO: make this different depending on which child
      //sort the file
      int length;
      struct record *records;
      records = sort("file_2.dat", &length);
      printStructArray(records,length);
      
      //register signal handler. Will be used by parent to request another record.
      signal(SIGUSR1, sendNextSig);
      //set up pipe descriptors
      dup2 (fd2[WRITE], STDOUT_FILENO);
      close (fd2[READ]);
      close (fd2[WRITE]);
      
      int count = 0;
      //stay alive until EOF
      while(1){
	if(sendNext){
	  //perror("beggining write");
	  //write up to parent
	  write(STDOUT_FILENO, &records[count], sizeof(struct record));
	  //send signal to parent
	  if (kill(getppid(),SIGUSR2) < 0){
	    perror("kill failed");
	    exit(1);
	  }
	  //perror("Signal sent to parent");
	  //set own signal to not ready
	  sendNext = 0;
	}
      }
    }
  } else if(pid1 == 0) { //I am child 1
    printf("I am child %ld, Parent is %ld\n", (long)getpid(), (long)getppid());
    
    //open up the file. TODO: make this different depending on which child
    //sort the file
    int length;
    struct record *records;
    printf("Child 1 sorting file\n");
    records = sort("file_1.dat", &length);
    printf("Child 1 done sorting file\n");
    printStructArray(records,length);
    
    //register signal handler. Will be used by parent to request another record.
    signal(SIGUSR1, sendNextSig);
    //set up pipe descriptors
    dup2 (fd1[WRITE], STDOUT_FILENO);
    close (fd1[READ]);
    close (fd1[WRITE]);
    
    int count = 0;
    //stay alive until EOF
    while(1){
      if(sendNext){
	//perror("beggining write");
	//write up to parent
	write(STDOUT_FILENO, &records[count], sizeof(struct record));
	//send signal to parent
	if (kill(getppid(),SIGUSR1) < 0){
	  perror("kill failed");
	  exit(1);
	}
	//perror("Signal sent to parent");
	//set own signal to not ready
	sendNext = 0;
      }
    }
  }
  
  //}
  
  return 0;
}

struct record* sort(char* fileName, int* length){
  char line[MAX];
  char* token;
  struct record temp;
  int count = 0;
  struct record *records;
  
  records = malloc(sizeof(struct record));
  
  FILE* file;
  file = fopen(fileName, "r");
  
  printf("file is open\n");
  
  while(fgets(line, MAX, file) != NULL) {
    printf("file is open 2\n");
    records = realloc(records, sizeof(struct record*) * count);
    
    token = strtok(line," ");
    temp->transactionNum = atoi(token);
    
    printf("file is open %d\n",temp->transactionNum);
    
    token = strtok(NULL," ");
    temp->customerNum = atoi(token);
    
    printf("file is open \n");
    
    token = strtok(NULL," ");
    temp->amount = atof(token);
    
    printf("file is open 5\n");
    
    records[count] = temp,sizeof(*temp));
    printf("file is open 6\n");
    count++;
    printf("Added new record to struct\n");
  }
  fclose(file);
  *length = count;
  return records;
}

void printStructArray(struct record* records, int length){
  int i;
  for(i=0;i<length;i++){
    printf("%d %d %lf\n",records[i].transactionNum,records[i].customerNum,records[i].amount);
  }
}

void sendNextSig (int sigNum)
{
  sendNext = 1;
}

void readReadySig1 (int sigNum)
{
  readReady1 = 1;
  //printf("ready 1 on\n");
}
void readReadySig2 (int sigNum)
{
  readReady2 = 1;
  //printf("ready 2 on\n");
}

