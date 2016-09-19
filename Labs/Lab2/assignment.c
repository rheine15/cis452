#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define LINESIZE 256

void display_prompt();
char** read_command();

int main(int argc, char* argv[])
{
  
  pid_t pid; 
  int status;
  char** command;
  
  while(1){
    command = read_command();
    
    //break if user has entered 'quit'
    if(command != NULL && !strcmp(command[0],"quit")){
      break;
    }
    
    pid = fork();
    
    if(pid < 0){
      perror("fork failed");
      exit(1);
    } else if(pid > 0){ //I am the parent
      waitpid(-1, &status, 0);
      //wait is over. get stats.
      struct rusage usage;
      if(getrusage(RUSAGE_CHILDREN,&usage)<0){
	perror("get usage failed");
	exit(1);
      }
      printf("CPU Time: %ld.%06ld\n",usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
      printf("Involuntary Context Switches: %ld\n",usage.ru_nivcsw);
    } else if(pid == 0) { //I am a child
      if(execvp(command[0], &command[0]) < 0) {
	perror("exec failed");
	exit(1);
      }     
    }
  }
  //exit if we have broken out of the loop on quit
  free(command);
  return 0;
}

void display_prompt(){
  printf("Command > ");
}

char** read_command(){ 
  
  char buffer[LINESIZE];
  char** command;
  char* word;
  
  //run until acceptable command is entered
  while(1){
    display_prompt();
    
    //receive line from user input
    fgets(buffer, LINESIZE , stdin);
    
    //no command was entered
    if(strlen(buffer)==1){
      continue;
    }
    
    //get rid of new line at end
    buffer[strlen(buffer)-1]='\0';
    
    //check if command entered was too long
    if (strlen(buffer) >= LINESIZE-2){
      //read remaining from stdin. Recieved from http://stackoverflow.com/questions/30811325/read-from-stdin-with-fgets-bug-if-input-is-greater-than-size
      int c;
      while((c = getc(stdin)) != '\n' && c != EOF);     
      printf("Error: Command was too long\n");
      //restart reading user input
      continue;
    }
    
    //command is acceptable
    break;
  }
 
  //read first token and setup memory for array
  command = malloc(sizeof(char*));
  word = strtok(buffer," ");
  
  //loop through all of the arguments
  int count = 0;
  do{ 
    //reallocate size of pointer array
    command = realloc(command,sizeof(char*) * (count+1));
    //allocate space for string that is being pointed to
    command[count] = malloc((strlen(word)+1) * sizeof(char));
    //copy the string to our pointer array
    strcpy(command[count],word);
    //increment count for next iteration
    count++;
  } while((word = strtok(NULL," ")) != NULL);
  
  //null terminate the argument list for the exec
  command[count]=NULL;
  
  //free memory no longer needed
  free(word);
  
  return command;
}

