#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <math.h>

#define READ 0
#define WRITE 1
#define MAX 1024

struct record{
  int transactionNum;
  int customerNum;
  double amount;
};

void sendNextSig(int);
void readReadySig (int);
int calcUnique (char*);
struct record* load(char*,int*);
int cmpCustomerNum(const void*, const void*);
void printStructArray (struct record*,int);

//global variables for sigHandler
int sendNext = 1;
int readReady = 0;

int main(int argc, char* argv[])
{
  pid_t pid1, pid2;
  int fd_up[2],fd_left[2],fd_right[2];
  char fileNames [8][15] = {"file_1.dat\0","file_2.dat\0","file_3.dat\0","file_4.dat\0","file_1.dat\0","file_2.dat\0","file_3.dat\0","file_4.dat\0"};
  
  //Receive number of file from user. Command line or runtime
  int numFiles = -1;
  if(argc>1){
    numFiles = strtol(argv[1],NULL,10);
  }
  while(numFiles == 1 || (numFiles & (numFiles - 1)) != 0){
    printf("Please input a power of 2: ");
    scanf("%d", &numFiles);
  }
  
  //Set up number of levels based on number of files
  int maxLevel = (int) log2(numFiles);
  //set up 'binary' string
  char unique [maxLevel];
  
  printf("Number of levels: %d for %d files\n",maxLevel,numFiles);
  int level;
  
  for (level = 0; level < maxLevel; level++){
    
    //set up pipe for first child
    if (pipe(fd_left) < 0) {
      perror ("plumbing problem");
      exit(1);
    }
    
    if ((pid1 = fork()) < 0){
      perror("Fork failure\n");
      exit(1);
    }else if(pid1 > 0){ // I am the parent
      if (pipe(fd_right) < 0) { //pipe for second child
	perror ("plumbing problem");
	exit(1);
      }
      if((pid2 = fork()) < 0){ //fork for second child
	perror("Fork failure\n");
	exit(1);
      } else if(pid2 > 0){ // I am the parent
	//close unneeded descriptors
	close(fd_left[WRITE]);
	close(fd_right[WRITE]);
	//register signal handler to receive from child.
	signal(SIGUSR2, readReadySig);
	break;
      }else { //I am right child.
	//set value for unique index
	unique[level]='1';
	//save value for write pipe
	//TODO: dup2(fd_right[WRITE],fd_up[WRITE]);
	fd_up[WRITE] = fd_right[WRITE];
	//close unneeded descriptors
	close(fd_left[READ]);
	close(fd_right[READ]);
	close(fd_left[WRITE]);
	//register signal handler to receive from parent.
	signal(SIGUSR1, sendNextSig);
	continue;
      }   
    }//I am left child
    //set value for unique index
    unique[level]='0';
    //save value for write pipe
    //TODO:dup2(fd_left[WRITE],fd_up[WRITE]);
    fd_up[WRITE] = fd_left[WRITE];
    //close unneeded descriptors
    close(fd_left[READ]);
    close(fd_right[READ]);
    close(fd_right[WRITE]);
    //register signal handler to receive from parent.
    signal(SIGUSR1, sendNextSig);
  }
  
  //our process trees and pipes are initialized
  if(level != maxLevel){ //I am a merger/master
    if(level == 0){
      //TODO: dup2 for fd_up to go to file
      close(fd_up[WRITE]);
      printf("Master\tLevel %d. I am %d. My parent is %d. My children are %d and %d\n", level,(int)getpid(), (int)getppid(),(int)pid1,(int)pid2);
    } else {
      printf("Merger\tLevel %d. I am %d. My parent is %d. My children are %d and %d\n", level,(int)getpid(), (int)getppid(),(int)pid1,(int)pid2);
    }
    
    ssize_t num1, num2; 
    int lastRead = 0;
    struct record temp1;
    struct record temp2;
    
    //stay alive to continue reading
    while(1) { //TODO: rethink this logic?
      if(readReady == 2 && sendNext){
	//printf("%d: beginning reads\n",getpid());
	if(lastRead != 1){
	  //printf("%d: reading left\n",getpid());
	  num1 = read(fd_left[READ],(void *)&temp1, (size_t)sizeof(struct record));     
	}
	if(lastRead != 2){
	  //printf("%d: reading right\n",getpid());
	  num2 = read(fd_right[READ],(void *)&temp2, (size_t)sizeof(struct record));
	}
	//check if there was an error
	if(num1 > MAX || num2 > MAX) {
	  perror ("pipe read error\n");
	  exit(1);
	}else if(num1 == 0 || num2 == 0){ //TODO: only exit if both are done
	  printf("%d: I am done. Goodbye!\n",(int)getpid());
	  exit(0);
	}
	printf("%d: reads done\n",getpid());
	
	//compare our two customer numbers
	if(temp1.customerNum < temp2.customerNum){
	  write(fd_up[WRITE], &temp1, sizeof(struct record));
	  printf("%d: %d\t%d\t%lf\n",(int)getpid(),temp1.transactionNum,temp1.customerNum,temp1.amount);
	  lastRead = 2;
	  readReady = 1;
	  //request another record.
	  if (kill(pid1,SIGUSR1) < 0){
	    perror("kill failed");
	    exit(1);
	  }
 	  printf("%d: Requesting from child %d\n",(int)getpid(), (int)pid1);  
	} else {
	  write(fd_up[WRITE], &temp2, sizeof(struct record));
	  printf("%d: %d\t%d\t%lf\n",(int)getpid(),temp2.transactionNum,temp2.customerNum,temp2.amount);
	  lastRead = 1;
	  readReady = 1;
	  //request another record.
	  if (kill(pid2,SIGUSR1) < 0){
	    perror("kill failed");
	    exit(1);
	  }
	  printf("%d: Requesting from child %d\n",(int)getpid(), (int)pid2);  
	}
	if(level > 0){ //I am a merger
	  //send signal to parent
	  if (kill(getppid(),SIGUSR2) < 0){
	    perror("kill failed");
	    exit(1);
	  }
	  //set own signal to not ready
	  sendNext = 0;
	}
      }//end of send
    }//end of stay alive 
  } else{ //I am a leaf
    //get our unique index to know which file to sort
    int index = calcUnique(unique);
    printf("Leaf%d\tLevel %d. I am %d. My parent is %d.\n", index, level,(int)getpid(), (int)getppid());
    
    int numRecords;
    struct record *records;
    
    //obtain records and sort file
    //printf("Child %d loading structs from file\n",(int)getpid());
    records = load(fileNames[index], &numRecords);
    //printf("Child %d sorting structs\n",(int)getpid());
    qsort(records, numRecords, sizeof(struct record), cmpCustomerNum);
    //printf("Child %d done sorting structs\n",(int)getpid());
    //printStructArray(records,numRecords);
    
    int count = 0;
    //stay alive until EOF to send one record at a time
    while(1){
      if(sendNext){
	//write up to parent
	write(fd_up[WRITE], &records[count], sizeof(struct record));
	//printf("%d: Sent customerNum %d to %d\n",(int)getpid(),records[count].customerNum,(int)getppid());
	//send signal to parent
	if (kill(getppid(),SIGUSR2) < 0){
	  perror("kill failed");
	  exit(1);
	}
	//set own signal to not ready
	sendNext = 0;
	count++;
	if(count >= numRecords){ //TODO: is this how to exit??
	  printf("%d: I am done. Goodbye!\n",(int)getpid());
	  //free memory from records and exit
	  free(records);
	  exit(0);
	}
	
	//printf("%d: Waiting for signal from parent\n",(int)getpid());
      }
    }
  }
  return(0);
}

struct record* load(char* fileName, int* pNumRecords){
  char line[MAX];
  char* token;
  int count = 0;
  struct record *records;
  struct record temp;
  
  records = (struct record *)malloc(sizeof(struct record) * (count+1));
  
  FILE* file;
  //printf("Opening %s\n",fileName);
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
  *pNumRecords = count;
  return records;
}

int cmpCustomerNum(const void *a, const void *b){ 
  //cast pointer type
  struct record *ia = (struct record *)a;
  struct record *ib = (struct record *)b;
  
  return ia->customerNum - ib->customerNum;  
}

int calcUnique(char* binary){
  int mult = 1;
  int sum = 0;
  int i;
  for(i=0;i<strlen(binary);i++){
    if(binary[i]=='1'){sum+=mult;}
    mult*=2;
  }
  return sum;
}

void printStructArray(struct record *records, int length){
  printf("Struct Array (Length %d):\n",length);
  int i;
  for(i=0;i<length;i++){
    printf("%d\t%d\t%lf\n",records[i].transactionNum,records[i].customerNum,records[i].amount);
  }
}

void sendNextSig(int sigNum)
{
  sendNext = 1;
  //printf("%d: signal received from %d\n",(int)getpid(),(int)getppid());
}
void readReadySig (int sigNum)
{
  readReady++;
  printf("%d: readyReady %d\tsendNext %d\n",(int)getpid(),readReady,sendNext);
}



