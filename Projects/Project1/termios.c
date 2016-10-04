#include <stdio.h>
#include <termios.h>

#define LINESIZE 256

int main(int argc, char* argv[])
{
    //declare variables for termios attributes and strings
    struct termios old_attr, new_attr;
    char secret[LINESIZE];
    char visible[LINESIZE];
    
    //get attributes for the 'stdin' file
    tcgetattr(fileno(stdin),&old_attr);
    //set the new attributes to the old attributes
    new_attr = old_attr;
    
    printf("\nDisabling echo.\n");
    //modify the echo flag on new attributes
    new_attr.c_lflag &= ~ECHO;
    //set the new attributes
    tcsetattr(fileno(stdin), TCSANOW, &new_attr);
    
    printf("Enter secret word/phrase: ");
    //read from user input
    fgets(secret,LINESIZE,stdin);
    //print what was read from stdin
    printf("\nYou entered: %s\n",secret);
    
    //reset termios attributes
    tcsetattr(fileno(stdin), TCSANOW, &old_attr);
    printf("Default behavior restored.\n");
    
    printf("Enter visible word/phrase: ");
    //read from user input
    fgets(visible,LINESIZE,stdin);
    
    //exit the program
    printf("\n");   
    return 0;
}
