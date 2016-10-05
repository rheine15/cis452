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
struct record* load(char*,int*);
void printStructArray (struct record*,int);
int cmpCustomerNum(const void*, const void*);

//global variables for sigHandler
int sendNext = 1;
int readReady1 = 0;
int readReady2 = 0;

int main(int argc, char* argv[])
{
  pid_t pid1, pid2;
  int fd1[2], fd2[2];
  ssize_t num1, num2; 
  
  //TODO: stay alive to fork multiple times depending on number of files
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
      printf("I am the merger %ld\n",(long)getpid());
      //register signal handler. Will be sent from child to let know have loaded another input.
      signal(SIGUSR1, readReadySig1);
      signal(SIGUSR2, readReadySig2);
      
      //close unneccesary file descriptors
      close (fd1[WRITE]);
      close (fd2[WRITE]);
      
      //keep track of which pipe to read from
      int lastRead = 0;
      struct record temp1;
      struct record temp2;
      
      //stay alive
      while(1) {
	if(readReady1 && readReady2){
	  //printf("Beginning read\n");
	  //read a single record from the pipe
	  if(lastRead != 2){
	    num1 = read(fd1[READ],(void *)&temp1, (size_t)sizeof(struct record));
	  }
	  if(lastRead != 1){
	    num2 = read(fd2[READ],(void *)&temp2, (size_t)sizeof(struct record));
	  }
	  //check if there was an error
	  if (num1 > MAX || num2 > MAX) {
	    perror ("pipe read error\n");
	    exit(1);
	  }
	  
	  if(temp1.customerNum < temp2.customerNum){
	    printf("%d\t%d\t%lf\n",temp1.transactionNum,temp1.customerNum,temp1.amount);
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
	    printf("%d\t%d\t%lf\n",temp2.transactionNum,temp2.customerNum,temp2.amount);
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
      } //end of while
    } else if(pid2 == 0){ //I am child 2
      printf("I am child %ld, Parent is %ld\n", (long)getpid(), (long)getppid());
      
      //TODO: make filename different depending on which child
      //sort the file
      int length;
      struct record *records;
      printf("Child 2 loading structs from file\n");
      records = load("file_2.dat", &length);
      printf("Child 2 sorting structs\n");
      qsort(records, length, sizeof(struct record), cmpCustomerNum);
      printf("Child 2 done sorting file\n");
      //printStructArray(records,length);
      
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
	  //write up to parent
	  write(STDOUT_FILENO, &records[count], sizeof(struct record));
	  //send signal to parent
	  if (kill(getppid(),SIGUSR2) < 0){
	    perror("kill failed");
	    exit(1);
	  }
	  count++;
	  if(count >= length){
	    break;
	  }
	  //set own signal to not ready
	  sendNext = 0;
	}
      }
      //free memory from records and exit
      free(records);
      exit(0);
    }
  } else if(pid1 == 0) { //I am child 1
    printf("I am child %ld, Parent is %ld\n", (long)getpid(), (long)getppid());
    
    //TODO: make filename different depending on which child
    //sort the file
    int length;
    struct record *records;
    printf("Child 1 loading structs from file\n");
    records = load("file_1.dat", &length);
    printf("Child 1 sorting structs\n");
    qsort(records, length, sizeof(struct record), cmpCustomerNum);
    printf("Child 1 done sorting file\n");
    //printStructArray(records,length);
    
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
	//write up to parent
	write(STDOUT_FILENO, &records[count], sizeof(struct record));
	//send signal to parent
	if (kill(getppid(),SIGUSR1) < 0){
	  perror("kill failed");
	  exit(1);
	}
	count++;
	if(count >= length){
	  break;
	}
	//set own signal to not ready
	sendNext = 0;
      }
    }
    //free memory from records and exit
    free(records);
    exit(0);
  }
  
  //}
  
  return 0;
}

struct record* load(char* fileName, int* length){
  char line[MAX];
  char* token;
  int count = 0;
  struct record *records;
  struct record temp;
  
  records = (struct record *)malloc(sizeof(struct record) * (count+1));
  
  FILE* file;
  file = fopen(fileName, "r");
  
  while(fgets(line, MAX, file) != NULL) {
    //add an element to the array
    records = (struct record *)realloc(records, sizeof(struct record) * (count+1));
    
    token = strtok(line," ");
    temp.transactionNum = atoi(token);
    
    token = strtok(NULL," ");
    temp.customerNum = atoi(token);
    
    token = strtok(NULL," ");
    temp.amount = atof(token);
    
    //copy temp to the array
    memcpy(&records[count],&temp,sizeof(struct record));
    
    count++;
  }
  fclose(file);
  *length = count;
  return records;
}

void printStructArray(struct record *records, int length){
  printf("Struct Array (Length %d):\n",length);
  int i;
  for(i=0;i<length;i++){
    printf("%d\t%d\t%lf\n",records[i].transactionNum,records[i].customerNum,records[i].amount);
  }
}

int cmpCustomerNum(const void *a, const void *b) 
{ 
  //cast pointer type
  struct record *ia = (struct record *)a;
  struct record *ib = (struct record *)b;
  
  return ia->customerNum - ib->customerNum;
  
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

