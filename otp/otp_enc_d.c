/*********************************************************************
** Author: Romano Garza
** Date: 10/30/17
** Description: Server program for receiving plain txt and a key from
** a specific client and encrypting the plain text and sending it back 
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
    int temp, characterK, characterT, checkSend, total;
    socklen_t sizeOfClientInfo;
    struct sockaddr_in serverAddress, clientAddress;
    char buffer[256], biteSize[500];

    // check usage and arguments
    if(argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); }
    
    // Here, we set up the address struct for the server process
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
            // first we'll check that server is communicating with
            // 'otp_enc' and not some other server. to do that:
            
            // clear out buffer
            memset(buffer, '\0', 256);
            // get the servers message
            charsRead =  (int)recv(establishedConnectionFD, buffer, 255, 0);
            if(charsRead <  0) error("Server: ERROR reading from socket");
            
            // check if it is the right message, from the right client
            if(strcmp(buffer, "Yo, it's me the client") ==0)
            {
                
                //valid client, send back appropriate server message
                charsRead = (int)send(establishedConnectionFD, "Oh shoot waddup?", 18, 0);
                if(charsRead <  0) error("Server: ERROR writing to socket");
                
                
                //server will send plain txt first, clear out plain txt buffer
                memset(bufferTxt, '\0', sizeof(bufferTxt));
                // get the clients plain txt
                // loop must be used to prevent cutting off early
                // @ is the end flag to check for
                while(strstr(bufferTxt, "@") == NULL)
                {
                    // grab biteSize pieces from the incoming socket
                    memset(biteSize, '\0', sizeof(biteSize));
                    // recv and append to workbuffer
                    charsRead = (int)recv(establishedConnectionFD, biteSize, sizeof(biteSize)-1,0);
                    if (charsRead == -1)error("SERVER: ERROR reading txt from client");
                    strcat(bufferTxt, biteSize);
                }
                // get rid of @ symbol
                bufferTxt[strlen(bufferTxt) - 1] = '\0';
                
                
                //send the client back a message letting them know full txt was received
                charsRead = (int)send(establishedConnectionFD, "Got it!", 8, 0);
                if(charsRead <  0) error("Server: ERROR writing to socket");

                
                // clear out bufferKey, for incoming key file data
                memset(bufferKey, '\0', sizeof(bufferKey));
                // get the clients key txt in the same way the txt was received
                while (strstr(bufferKey, "@") == NULL)
                {
                    memset(biteSize, '\0', sizeof(biteSize));
                    // recv and append to workbuffer
                    charsRead = (int)recv(establishedConnectionFD, biteSize, sizeof(biteSize)-1,0);
                    if (charsRead == -1) error("SERVER: ERROR reading key from client");
                    strcat(bufferKey, biteSize);
                }
                // get rid of @ symbol
                bufferKey[strlen(bufferKey) - 1] = '\0';
                
                
                //send confirmation of receipt
                charsRead = (int)send(establishedConnectionFD, "Got it!", 8, 0);
                if(charsRead <  0) error("Server: ERROR writing to socket");
                
                // make sure the plain text is not larger than the key
                if(strlen(bufferTxt) > strlen(bufferKey)) error("Server: ERROR: key is too small");
                
                // clear out the cipher buffer
                memset(bufferCiph, '\0', sizeof(bufferCiph));
                
                // go through each string and encrypt via pad method
                for(i = 0; bufferTxt[i]!='\0'; i++){
                    // store character value as integers
                    characterK = (int)bufferKey[i];
                    characterT = (int)bufferTxt[i];
                    // make sure character value is not out of range i.e. bigger than Z
                    if(characterT > 90|| characterK > 90){
                        error("Server: Error: input contains bad character");
                    }
                    
                    // check if either character value is space
                    if(characterT == 32 || characterK == 32){
                        // if the plain text character is a space
                        // set it's numeric value to the space code numeric value
                        if(characterT == 32) characterT = 91;
                        // same for the key character
                        if(characterK == 32) characterK = 91;
                    }
                    // if neither are spaces, then none should be less than 65
                    else if(characterT < 65 || characterK < 65){
                        //printf("\nCharacter K: %c Character T: %c ", bufferKey, bufferTxt);
                        error("Server: Error: input contains bad character");
                    }
                    
                    // in thiss case both are capital letters so we subtract 65 twice
                    temp = characterT - 65 + characterK  - 65;
                    // if temp is >= 27, we must subtract 27 to get the mod value
                    if(temp >= 27)
                    {
                        temp = temp - 27;
                    }
                    // if the value is less than 27, it already is the mod value
                    
                    // we have to check if temp is 26, the space value
                    if(temp == 26)
                    {   // if it is then we add the space character to the index location
                        temp = 32;
                        bufferCiph[i] = (char) temp;
                        //printf("T: %c \nK: %c \nC: %c\n", (char)characterT, (char)characterK, (char)temp);
                        // printf("T:%d K:%d Tot: %d Temp: %d\n\n", characterT, characterK, total, temp);
                    }
                    else{
                        // if temp is not a space, we can simply add 65 to it to get it to
                        // be a capital letter
                        temp = temp + 65;
                        bufferCiph[i] = (char)temp;
                        //printf("T: %c \nK: %c \nC: %c\n", (char)characterT, (char)characterK, (char)temp);
                        // printf("T:%d K:%d Tot: %d Temp: %d\n\n", characterT, characterK, total, temp);
                    }
                    
                }
                //we add a flag to the end
                bufferCiph[i] = '@';
                
                //wait for request from server before sending cipher text
                memset(buffer, '\0', sizeof(buffer));
                charsRead = (int)recv(establishedConnectionFD, buffer, sizeof(buffer)-1,0);
                if(charsRead <  0) error("Server: ERROR writing to socket");
                
                // send the cipher
                charsRead = (int)send(establishedConnectionFD, bufferCiph, sizeof(bufferCiph) + 1, 0);
                if(charsRead <  0) error("Server: ERROR writing to socket");

                //shutdown the connection
                shutdown(establishedConnectionFD, 2);
                exit(0);
            }
            else{
                //invalid client, send back appropriate server message
                charsRead = (int)send(establishedConnectionFD, "\nServer otp_enc_d's message:\nNew phone who dis?\n", 51, 0);
                if(charsRead < 0) error("ERROR writing to socket");
            }
            // child is done
        }
        // back in parent
    
    }
    return 0;
}
