/*********************************************************************
 ** Author: Romano Garza
 ** Date: 10/30/17
 ** Description: Server program for receiving cipher txt and a key from
 ** a specific client and decrypting the cipher and sending it back
 ** to client
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>

char bufferTxt[100000], bufferKey[100000], bufferCiph[100000];
void error(const char *msg) { perror(msg); exit(EXIT_FAILURE); } // Error function used
// for reporting issues

int main(int argc, const char * argv[]) {
    // standard socket variables
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead, charsReadKey, charsReadTxt, pid, i, size;
    int temp, remainder, characterK, characterC, checkSend;
    socklen_t sizeOfClientInfo;
    struct sockaddr_in serverAddress, clientAddress;
    char buffer[256], biteSize[500];

    
    // check usage and arguments
    if(argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); }
    
    // Here, we set up the address struct for the server process (this one):
    // clear out the address struct
    memset((char *)&serverAddress, '\0', sizeof(serverAddress));
    // get port # and convert to int from string
    portNumber = atoi(argv[1]);
    // create a socket with network capability
    serverAddress.sin_family = AF_INET;
    // store the port number
    serverAddress.sin_port = htons(portNumber);
    // allow any dress for connections to this server process
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    
    // Next, set up the socket:
    // create the socket, check for errors
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocketFD < 0) error("Server: ERROR opening socket");
    // enable socket to begin listening
    if(bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
        error("Server: ERROR on binding");
    }
    // flip socket on and all to receive up to 5 connects
    listen(listenSocketFD, 5);
    
    // accept a connection blocking if one is not available until one connects
    sizeOfClientInfo = sizeof(clientAddress);
    
    while(1){
        // accept and check for errors
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if(establishedConnectionFD <  0) error("Server: ERROR on accept");
        
        // fork
        pid = fork();
        // check for fork errors
        if(pid < 0) error("Server: Error on fork");
        else if(pid == 0){
            // inside child process
            
            // clear out buffer
            memset(buffer, '\0', 256);
            // get the clients message
            charsRead =  (int)recv(establishedConnectionFD, buffer, 255, 0);
            if(charsRead <  0) error("Server: ERROR reading from socket");
            
            // validate the clients request
            if(strcmp(buffer, "You got the plain text?") ==0)
            {
                //valid client, so send back appropriate server message
                charsRead = (int)send(establishedConnectionFD, "You got the code?", 18, 0);
                //check for writing errors
                if(charsRead <  0) error("Server: ERROR writing to socket");
                
                // clear out the cipher buffer
                memset(bufferCiph, '\0', sizeof(bufferCiph));
                // get the clients cipher and check for errors
                while (strstr(bufferCiph, "@") == NULL)
                {
                    memset(biteSize, '\0', sizeof(biteSize));
                    // recv and append to workbuffer
                    charsRead = (int)recv(establishedConnectionFD, biteSize, sizeof(biteSize)-1,0);
                    if (charsRead == -1)
                    {
                        printf("%d", charsRead);
                        error("SERVER: ERROR reading txt from client");
                    }
                    strcat(bufferCiph, biteSize);
                }
                // get rid of @ symbol
                bufferCiph[strlen(bufferCiph) - 1] = '\0';
                
                // send back receipt confirmation
                charsRead = (int)send(establishedConnectionFD, "Got it!", 8, 0);
                if(charsRead <  0) error("Server: ERROR writing to socket");


                
                // clear out the key buffer
                memset(bufferKey, '\0', sizeof(bufferKey));
                // get the key from client
                while (strstr(bufferKey, "@") == NULL)
                {
                    
                    memset(biteSize, '\0', sizeof(biteSize));
                    // recv and append to buffer
                    charsRead = (int)recv(establishedConnectionFD, biteSize, sizeof(biteSize)-1,0);
                        if (charsRead == -1) error("SERVER: ERROR reading key from client");
                    strcat(bufferKey, biteSize);
                }
                // get rid of @ symbol
                bufferKey[strlen(bufferKey) - 1] = '\0';
                
                // send back receipt confirmation
                charsRead = (int)send(establishedConnectionFD, "Got it!", 8, 0);
                if(charsRead <  0) error("Server: ERROR writing to socket");
            
                // make sure the cipher text is not larger than the key
                if(strlen(bufferCiph) > strlen(bufferKey)) error("Server: ERROR: key is too small");
                else{
                    // cipher is good to be decrypted
                    // clear out the text buffer
                    memset(bufferTxt, '\0', sizeof(bufferCiph));
                    // go through each string and decrypt via pad method
                    for(i = 0; bufferCiph[i]!='\0'; i++){
                        // store character value as integers
                        characterK = (int) bufferKey[i];
                        characterC = (int) bufferCiph[i];
                        // make sure character value is not out of range i.e. bigger than Z
                        if(characterC > 90|| characterK > 90){
                            error("Server: ERROR: input contains bad character");
                        }
                        
                        // check if either character value is space
                        if(characterC == 32 || characterK == 32){
                            // if the plain text character is a space
                            // set it's numeric value to the space code numeric value
                            if(characterC == 32) characterC = 91;
                            // same for the key character
                            if(characterK == 32) characterK = 91;
                        }
                        // if neither are spaces, then none should be less than 65
                        else if(characterC < 65 || characterK < 65){
                            error("Server: ERROR: input contains bad character");
                        }
                        
                        // if here, then either both are capital letters, both are spaces, or 1 is space
                        // we start by checking if both are spaces
                        // in this case both are capital letters so we subtract 65 twice
                        temp = (characterC - 65) - (characterK - 65);
                        // if temp is less than 0 we add 27 to it to get the mod value
                        if(temp < 0)
                        {
                            temp = temp + 27;
                        }
                        // else we can leave temp alone because it _is_ the mod value
                        
                        //printf("c: %d k:%d ab:%d temp: %d\n", characterC, characterK, temp);
                        // we have to check if temp is 26, the space value
                        if(temp == 26)
                        {   // if it is then we add the space character to the index location
                            temp = 32;
                            bufferTxt[i] = (char)temp;
                            //printf("C: %c \nK: %c \nC: %c\n\n", (char)characterC, (char)characterK, (char)temp);
                            
                        }
                        else{
                            // if temp is not a space, we can simply add 65 to it to get it to
                            // be a capital letter
                            temp = temp + 65;
                            bufferTxt[i] = (char)temp;
                            //printf("C: %c \nK: %c \nC: %c\n\n", (char)characterC, (char)characterK, (char)temp);
                        }
                    }
                    
                    bufferTxt[i] = '@';
                    //wait for confirmation to send plain text
                    memset(buffer, '\0', sizeof(buffer));
                    charsRead = (int)recv(establishedConnectionFD, buffer, sizeof(buffer)-1,0);
                    if(charsRead <  0) error("Server: ERROR writing to socket");
                    
                    //plain text can now be sent
                    charsRead = (int)send(establishedConnectionFD, bufferTxt, sizeof(bufferTxt) + 1, 0);
                        
                    // close connection and exit*/
                    shutdown(establishedConnectionFD, 2);
                    exit(0);
                }
            }
            else{
                //invalid client, send back appropriate server message
                charsRead = (int)send(establishedConnectionFD, "Server otp_dec_d's message:\nNew phone, who dis?\n", 51, 0);
                if(charsRead < 0) error("ERROR writing to socket");
            }
            // child is done
        }
        // back in parent
        
    }
    return 0;
}
