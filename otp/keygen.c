/*********************************************************************
 ** Author: Romano Garza
 ** Date: 10/30/17
 ** Description: Program that generates a random key 
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

int main(int argc, const char * argv[]) {
    int i;
    char randomInt, randomChar;
    int count = atoi(argv[1]);
    
    // buffer variable
    char buffer[100000];
    
    // seed random
    srand((unsigned int)time(0));
    // clear out the buffer
    memset(buffer, '\0', sizeof(buffer));
    // generate random numbers leaving room for potential new line
    for(i = 0; i < count; i++){
        //get a random number between 0-26
        randomInt = rand()%27;
        // if 26, then it's a space
        if(randomInt == 26)
        {
            randomChar = (char)32;
        }
        else{
            randomChar = (char)(randomInt + 65);
        }
        buffer[i]=randomChar;
    }
    buffer[strlen(buffer)] = '\n';
    buffer[strlen(buffer)+1] = '\0';

    
    fprintf(stdout, "%s", buffer);
    return 0;
}
