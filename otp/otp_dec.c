/*********************************************************************
** Author: Romano Garza
** Date: 10/30/17
** Description: Client program for sending cipher txt and a key to
** a specific server that will decrypt the cipher text via the key and
** send it back
*********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <time.h>
#include <sys/ioctl.h>


void error(const char *msg) { perror(msg); exit(EXIT_FAILURE); } // Error function used for reporting issues


int main(int argc, char *argv[])
{
    int socketFD, portNumber, charsWritten, charsRead, characterK, characterC, i, checkSend;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;
    char buffer[256], biteSize[500], bufferTxt[100000], bufferKey[100000], bufferCiph[100000];
    long fsize, fsize1;
    if (argc < 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args
    
    // Set up the server address struct
    memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    
    
    // before contacting server, perform all the
    // file operations and validation
    // this will help with error checking and preventing server from
    // having any uneccessary errors improper error
    
    
    FILE *fp = fopen(argv[1], "rb");
    if (fp != NULL) {
        //go to end of file
        fseek(fp, 0, SEEK_END);
        //get size
        fsize = ftell(fp);
        //rewind to begining of file
        fseek(fp, 0, SEEK_SET);
        
        //allocate space for txt string
        memset(bufferCiph, '\0', sizeof(bufferTxt));
        // bufferTxt = malloc(fsize + 1);
        fread(bufferCiph, fsize, 1, fp);
        if ( ferror(fp) != 0 ) {
            error("CLIENT: ERROR reading plain text file");
        }
        // strip new line, replace with end of line character
        bufferCiph[fsize-1] = '\0';
        // set end to null terminator
        bufferCiph[fsize] = '\0';
        fclose(fp);
    }
    else{
        error("CLIENT: ERROR opening plain text file");
    }
    
    
    // then open key txt file and save input
    FILE *fp1 = fopen(argv[2], "rb");
    if (fp1 != NULL) {
        //go to end of file
        fseek(fp1, 0, SEEK_END);
        //get size
        fsize1 = ftell(fp1);
        //rewind to begining of file
        fseek(fp1, 0, SEEK_SET);
        
        //allocate space for txt string
        memset(bufferKey, '\0', sizeof(bufferKey));
        //bufferKey = malloc(fsize1 + 1);
        fread(bufferKey, fsize1, 1, fp1);
        if ( ferror(fp1) != 0 ) {
            error("CLIENT: ERROR reading plain text file");
        }
        // strip new line, replace with null
        bufferKey[fsize1-1] = '\0';
        // set end to null terminator
        bufferKey[fsize1] = '\0';
        fclose(fp1);
    }
    else{
        error("CLIENT: ERROR opening key text file");
    }
    
    // check for size error
    if(strlen(bufferKey) < strlen(bufferCiph)) error("CLIENT: ERROR key text file is too short");
    
    // check for invalid characters in buffer txt
    for(i = 0; bufferCiph[i]!='\0'; i++){
        // store character value as integers
        characterC = (int) bufferCiph[i];
        // make sure character value is not out of range i.e. bigger than Z
        // or less than 65 and not 32(space)
        if(characterC > 90){
            memset(buffer, '\0', sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "%c", (char)characterC);
            error(buffer);
            error("CLIENT: ERROR: plain text input contains a bad character");
        }
        else if(characterC < 65 && characterC != 32){
            memset(buffer, '\0', sizeof(buffer));
            snprintf(buffer, sizeof(buffer), "%c", (char)characterC);
            error(buffer);
            error("CLIENT: ERROR: plain text input contains bad character");
        }
    }
    
    
    // check for invalid characters in buffer key
    for(i = 0; bufferKey[i]!='\0'; i++){
        // store character value as integers
        characterK = (int) bufferKey[i];
        // make sure character value is not out of range i.e. bigger than Z
        // or less than 65 and not 32(space)
        if(characterK > 90){
            error("CLIENT: ERROR: key text input contains bad character");
        }
        else if(characterK < 65 && characterK != 32){
            error("CLIENT: ERROR: key text input contains bad character");
        }
        
    }
    
    //replace ends with @ symbols
    bufferCiph[fsize-1] = '@';
    bufferKey[fsize1-1] = '@';
    // Now we can set up the connections:
    // we are only connecting to servers on te local host
    serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
    if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
    
    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address
    
    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (socketFD < 0) error("CLIENT: ERROR opening socket");
    
    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
        error("CLIENT: ERROR connecting");
    
    // Send greeting message to server
    charsWritten = send(socketFD, "You got the plain text?", 24, 0); // Write to the server
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
    if (charsWritten < 23) printf("CLIENT: WARNING: Not all data written to socket!\n");

    // Get return message from server
    memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
    if (charsRead < 0) error("CLIENT: ERROR reading from socket");
    
    // validate message returned, that it is the correct server
    if(strcmp(buffer, "You got the code?") == 0)
    {

        // send both buffers to the server, server expects the cipher buffer first
        charsWritten = (int)send(socketFD, bufferCiph, strlen(bufferCiph) + 1, 0); // Write to the server
        if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
        
        checkSend = -5;  // Bytes remaining in send buffer
        do
        {
            ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
            //printf("checkSend: %d\n", checkSend);  // Out of curiosity, check how many remaining bytes there are:
        }
        while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
        
        if (checkSend < 0)  // Check if we actually stopped the loop because of an error
            error("ioctl error");
        //get back confirmation message
        memset(buffer, '\0', sizeof(buffer));
        charsRead = (int)recv(socketFD, buffer, sizeof(buffer)-1, 0);
        if (charsWritten < 0) error("CLIENT: ERROR reading from socket");
        //send key buffer
        charsWritten = (int)send(socketFD, bufferKey, strlen(bufferKey) + 1, 0); // Write to the server
        if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
        
        checkSend = -5;  // Bytes remaining in send buffer
        do
        {
            ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
            //printf("checkSend: %d\n", checkSend);  // Out of curiosity, check how many remaining bytes there are:
        }
        while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
        
        if (checkSend < 0)  // Check if we actually stopped the loop because of an error
            error("ioctl error");
        //get confirmation message
        memset(buffer, '\0', sizeof(buffer));
        charsRead = (int) recv(socketFD, buffer, sizeof(buffer)-1, 0);
        
        // send response
        charsRead = (int) send(socketFD, "Got it!", 8, 0);
        if (charsRead < 0 ) error("CLIENT: ERROR writing to server");
        
        // get the clients cipher txt and check for errors
        memset(bufferTxt, '\0', sizeof(bufferTxt));
        while (strstr(bufferTxt, "@") == NULL)
        {
            memset(biteSize, '\0', sizeof(biteSize));
            // recv and append to workbuffer
            charsRead = recv(socketFD, biteSize, sizeof(biteSize)-1,0);
            if (charsRead == -1) error("CLIENT: ERROR reading from server");
            strcat(bufferTxt, biteSize);
        }
        // get rid of @ symbol
        bufferTxt[strlen(bufferTxt) -1] = '\n';
        bufferTxt[strlen(bufferTxt)] = '\0';
        if (charsRead < 0) error("CLIENT: ERROR reading from socket");
        // write the encrypted text to std out
        fprintf(stdout, "%s", bufferTxt);
    }
    else{
        memset(bufferTxt, '\0', sizeof(buffer));
        strcat(bufferTxt, "CLIENT: ERROR opt_dec failed to connect with server");
        strcat(bufferTxt, buffer);
        error(bufferTxt);
    }
    close(socketFD); // Close the socket

    return 0;
}
