
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 16

int main()
{
    char *data1, *data2, *tmp;
    int i;

    data1 = malloc (SIZE);
    data2 = malloc (SIZE);
    printf ("Please input your eos username: ");
    scanf ("%s", data1);
    tmp = data1;
    for (i=0; i<SIZE; i++)
       data2[i] = *(tmp++);
    printf ("original :%s:\n", data1);
    free (data1);
    printf ("copy :%s:\n", data2);
    free (data2);
    return 0;
}
