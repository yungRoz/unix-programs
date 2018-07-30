/*********************************************************************
** Program name: smallsh
** Author: Romano Garza
** Date: 11/06/2017
** Description: This is the main file for a shell program that allows
** for the redirection of standard input and standard output and suppports
** both foreground and background processes
*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>


// Used to tokenizing input line of arguments
#define TOKEN_DELIMITERS " \r\t\n\a"

// Built in commands
char* builtinStrings[] = {"exit", "cd", "status"};

// Function prototypes
void tokenUpThisLine(char* args[], char line[]); // split up the input line and add to array
void processArgs(char* args[]);  // called after arguments have been added to array
int hasBuiltInCmnd(char* args[]); // checks for built in command in arguments array
void runBuiltInCmnd(char* args[]); // runs built in commands
void processNonBuiltIns(char* args[]); // runs not built in commands
void freeUpMyArgs(char* args[]); // frees up the arguments array so it can be recycled
void processInBG(char* args[]); //processes arguments in background
void processInFG(char* args[]); // processes arguments in foreground
//void catchSIGINT(int signo); // catches SIGINT signal and stops child process
void catchSIGTSTP(int signo);  //catches SIGTSTP signal and switches block background option
void processCheck(); // checks each background pid in pidArr to find if it's finished
void removePID(int pos); // removes pid at position pid[Arr] pos
// helper function for debugging
void *getCurrentDir(void);

// Globals
char line[2048]; // for holding the line of input the user passes in
int status = 0; // for holding the return status of processes
int sigintBool = 0; // for holding the turth value for parent processes
int keepGoingBool = 1; // boolean to keep the main loop going, only 0 when exit is entered
int isBackGround = 0; // boolean to run process in background when & is specified
int backGroundNotBlocked = 1; // boolean to block or unblock background running capability
int in = 0; //boolean for when input redirection is specified
int out = 0; // boolean for when output redirection is specified
char inputFile[512]; //char string to hold inputFile argument
char outputFile[512]; // char string to hold outPutFile argument
pid_t pidArr[512]; // array to hold pid of background processes
int pidPos = 0; // variable to track pidArray size/pos
struct sigaction SIGINT_action = {0}; //for catching SIGINTS
struct sigaction SIGTSTP_action = {0}; //for catching SIGTSTPS
struct sigaction ignore_or_default_action = {0}; //for ignoring SIGINTS


int main(int argc, const char * argv[]) {
    
    char* arguments[512]; //for storing the array of arguments
    
    //initialize the line for input
    memset(line, '\0', sizeof(line));
    
    // set up signal handlers to catch sigstp and by default ignore sigint in parent processes
    
    // this will allow sigtstp to be caught and executed with correct
    // switching of the boolean value that allows background processes to be
    // executed or blocked
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    
    // by default ignore SIGINT signals
    ignore_or_default_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &ignore_or_default_action, NULL);
    
    
    // This is a loop that will keep going as long as the user
    // has not entered exit on the command line    
    while(keepGoingBool)
    {
        //first we check if there are any added pids to check
        //if there are, we check for them with the call to processCheck
        if(pidPos > 0 )
        {
            //check for finished processes
            processCheck();
            
        }
        
        // Next we print the commandline 
        // Get input and keep getting it while the user
        // has entered a comment (#) or blank line
        do
        {
            printf(": ");
            fflush(stdout);
            fgets(line, 2048, stdin);
        }while(line[0] == '\0' || line[0] == '\n' || line[0] == '#');
        
        // Split up the input to get each argument
        // and fill arguments array, returns 1 if background
        tokenUpThisLine(arguments, line);

        // This is where the main splitting of the program occurs
        // -in processArgs
        processArgs(arguments);
        
        // Here, the arguments array is freed up to be used again
        freeUpMyArgs(arguments);
        memset(line, '\0', sizeof(line));
        isBackGround = 0;
    }

    //Used for debugging arguments
    /*int i;
    for(i = 0; arguments[i] != NULL; i++)
    {
        printf("\n Element is: %s \n", arguments[i]);
    }*/
    return 0; 
}


/*********************************************************************
** Description: Takes an empty array of string arguments and a line
** of input. Then, breaks the line into tokens via a set of delimiters.
** Each token is an argument. Redirection symbols and file names are
** stored in global variables, all other arguments are stored in the args
** array of strings.
*********************************************************************/
void tokenUpThisLine(char* args[], char line[])
{
    // Get the first argument
    char* token = strtok(line, TOKEN_DELIMITERS);
    // Index Variable
    int pos = 0; // this will be used for adding arguments
                 // to the correct position in the array of argument
    
    // this is used in the for loop to check for
    int i;
    // Assume No Redirection
    in = 0, out = 0;
    // buffer for pid
    char buffer[50];
    char checkBuffer[400];
    // we assume the line arguments won't be executed in background
    // this is done to reset the isBackground global variable
    // after each command gets executed
    isBackGround = 0;
    
    
    // Clear out file names and buffers
    memset(inputFile, '\0', sizeof(inputFile));
    memset(outputFile, '\0', sizeof(outputFile));
    memset(buffer, '\0', sizeof(buffer));           // used for switching out $$ with process id
    memset(checkBuffer, '\0', sizeof(checkBuffer)); // used for switching out $$ with process id

    // Tokenize the string saving each argument not associated with redirection
    // in the passed array
    while(token != NULL)
    {
        // we are going to check if input or output direction
        // is specified. such redirections and file names will
        // not be included in the array of arguments 
        if(strcmp(token, "<") == 0){
            // token is < symbol
            // hence, input redirection is specified
            // however we won't add either the redirection symbol
            // or the input file to the array of arguments
            in = 1; // set input boolean variable to 1
                    // we'll use this later to check for redirection
                    // when arguments are processed
            
            // file name should be next token, grab it
            token = strtok(NULL, TOKEN_DELIMITERS);
            // copy the filename into the global inputFile string
            // to be used later when arguments are processed
            strcpy(inputFile, token);
            // grab the next token
            token = strtok(NULL, TOKEN_DELIMITERS);
        }
        else if(strcmp(token, ">") == 0){
            // token is > symbol
            // in this case, output redirection is specified
            // neither the redirection symbol nor the output file name
            // are added to the array
            out = 1; // set output boolean variable to 1
                     // we'll use this later to check for redirection
                     // when arguments are processed
            
            // file name should be next token, grab it
            token = strtok(NULL, TOKEN_DELIMITERS);
            // copy the file name into the global outputFile string
            // to be used later when arguments are processed
            strcpy(outputFile, token);
            // grab the next token
            token = strtok(NULL, TOKEN_DELIMITERS);
        }
        else{
            // token is not < or >
            // here, the token is understood to be a 'normal' argument
            // to be later processed, so it gets stored in the array
            // of arguments
            
            // allocate space in the array
            args[pos] = malloc(strlen(token) + 1);
            // copy the string to the array of arguments
            // at the next position
            strcpy(args[pos], token);
            // get next token
            token = strtok(NULL, TOKEN_DELIMITERS);
            // increment array position so when the next argument is
            // found it can be added to the next array index
            pos++;
        }
    }
    
    // save the last position as NULL
    // this will be useful in the for loops to come
    // when we need to check the argument array for something
    // or when we need to free the arguments array
    args[pos] = NULL;

    // check if last argument is &
    if(strcmp(args[pos-1], "&") == 0)
    {
        // the argument is & so we set isBackground boolean to 1
        // we'll use this truth value later when processing the arguments
        isBackGround = 1;
        free(args[pos-1]);
        args[pos-1] = NULL;
    }
    
    
    // in this for loop we replace $$ in any argument with process id
    for(i = 0; args[i] != NULL; i++)
    {
        
        // copy over the argument to the checkBuffer
        strcpy(checkBuffer, args[i]);
        // check for $$ in the string
        // strstr will give a pointer to the first occurence of $$
        // it will return NULL when no $$ is found
        char* pFirstOccurence = strstr(checkBuffer, "$$");
        
        if(pFirstOccurence!=NULL)
        {   // here, $$ has been found in the string
            // next, we're going to treat $$ as the process id
            // so we copy the process id into a different buffer
            sprintf(buffer, "%d", getpid());
            //and swap out the $$ for the process id
            strncpy(pFirstOccurence, buffer, sizeof(buffer));
  
            // now we want to copy over the new string to the array
            // so first we free the current array argument string
            free(args[i]);
            // alocate space for the new string
            args[i] = malloc(strlen(checkBuffer) + 1);
            // copy over the newString
            strcpy(args[i], checkBuffer);
        }
    }
}


/*********************************************************************
** Description: processArgs takes an array of string arguments. It
** checks if the arguments contain a built in command. If they do, the
** arguments are processed by runBuiltInCmnd. If they don't, the arguments
** are processed via a child process
*********************************************************************/
void processArgs(char* args[])
{
    
    if(hasBuiltInCmnd(args)) // hasBuiltInCmnd returns 1 when the array of arguments
    {                        // contains cd, exit, or status
        // process the built in cmnd arguments
        runBuiltInCmnd(args);
    }
    else
    {   // here the arguments array does not have cd, exit, or status so they get
        // processed via processNonBuiltIN
        processNonBuiltIns(args);
    }    
}


/*********************************************************************
** Description: processNonBuiltIns takes an array of string arguments. 
** If the arguments have been specified to run in the background and 
** the background processes are not blocked, the arguments are processed
** in the background. If not, the arguments are processed in the foreground.
*********************************************************************/
void processNonBuiltIns(char* args[])
{
    // either process arguments in the background if specified, and not blocked
    // or process arguments in the foreground
    if(isBackGround && backGroundNotBlocked)
    {                                       // backGroundNotBlocked returns 1 when not blocked
        processInBG(args);                 // (see SIGTSTP_action())
                                               // isBackground returns 1 when user has entered & as last argument
    }
    else
    {   // process in foregroung
        processInFG(args); 
    }
        
}


/*********************************************************************
** Description: processInFG forks a child process which does file 
** redirection before calling execvp on the array of arguments. The
** parent process waits for the child to return before continuing
*********************************************************************/
void processInFG(char* args[])
{
    pid_t spawnPid = -5; 
    status = 0; // reset the global status variable
    int fd0, fd1; // ints for holding the return value of opening
                    // files for input or output redirection
	    
    // fork 
    spawnPid = fork();
    
    //check for errors, with fork()
    if(spawnPid == -1)
    {  //here, an error occurred
        perror("Hull Breach!\n");
        fflush(stdout); 
        exit(1); 
    }
    else if(spawnPid == 0)
    { // in this case, we're in the child process
        // enable SIGINT default so that SIGINT can be caught
        //ignore_or_default_action.sa_handler = SIG_DFL;
        //sigaction(SIGINT, &ignore_or_default_action, NULL);
        
        ignore_or_default_action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &ignore_or_default_action, NULL);
        
        // this will allow sigints to be caught and executed with correct
        // message sent to std out
        /*SIGINT_action.sa_handler = catchSIGINT;
        sigfillset(&SIGINT_action.sa_mask);
        SIGINT_action.sa_flags = 0;
        sigaction(SIGINT, &SIGINT_action, NULL);
        sigintBool = 1;*/
        // check for file input redirection
        if(in)
        {
            // open file for reading
            fd0 = open(inputFile, O_RDONLY); 
            // check for file open errors
            if(fd0 < 0)
            {
                printf("cannot open %s for input\n", inputFile);
                fflush(stdout);
                exit(1);
            }
            else
            {   // no errors, so dup2 input file, and redirect std input
                dup2(fd0, STDIN_FILENO);
                close(fd0);
            }
        }

        // check for file output redirection
        if(out)
        {
            // open file for writing to
            fd1 = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            // again, check for file open errors
            if(fd1 < 0)
            {
                // notify the user of any errors
                printf("cannot open %s for output\n", outputFile);
                fflush(stdout);
                exit(1);
            }
            else
            {  // no errors, so dup2 output file, and redirect std output
                dup2(fd1, STDOUT_FILENO);
                close(fd1);
            }
        }

        // ready to execute the command argument list
        if(execvp(args[0], args) < 0)
        {
            // if a command argument entered is not a command
            printf("%s: no such file or directory\n", args[0]);
            fflush(stdout);
            exit(1);
        }
        fflush(stdout);
        
        
    }
    //wait in parent for child to return
    waitpid(spawnPid, &status, 0);
    if(WIFEXITED(status))
    {
        //printf("exit value %d\n", WEXITSTATUS(status));
    }
    else
    {
        printf("terminated by signal %d\n", status);
    }
}


/*********************************************************************
** Description: processInBG forks a child process which does file
** redirection before calling execvp on the array of arguments. The
** parent process, adds the process id of the child to a global array,
** and continues taking commands back in main
*********************************************************************/
void processInBG(char* args[])
{
    pid_t spawnPid = -5; // variable to hold process id of child
    status = -5;         // reset global status varible
    int fd0, fd1;
    
    // fork
    spawnPid = fork(); 
    // check for errors
    if(spawnPid == -1)
    {
        // if there's an error, print statement
        perror("Hull Breach! \n"); 
        fflush(stdout);
        exit(1);
    }
    else if(spawnPid == 0)
    { // here, we're in the child
        // check if input file redirection has been specified
        /*earlier today, I moved*/
        ignore_or_default_action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &ignore_or_default_action, NULL);
        
        // this will allow sigints to be caught and executed with correct
        // message sent to std out
        /*SIGINT_action.sa_handler = catchSIGINT;
        sigfillset(&SIGINT_action.sa_mask);
        SIGINT_action.sa_flags = 0;
        sigaction(SIGINT, &SIGINT_action, NULL);*/
        
        if(in)
        {
            // open the file for reading;
            fd0 = open(inputFile, O_RDONLY);
            if(fd0 < 0)
            {// if there's an error, print nothing because process is in background
                // exit 1,
                exit(1);
            }
            else
            {
                dup2(fd0, STDIN_FILENO);
                close(fd0);
            }
        }
        else // input file has not been specified
        {
            //standard input is redirected from /dev/null
            fd0 = open("/dev/null", O_RDONLY);
            // check for file open errors
            if(fd0 < 0)
            {
                exit(1);
            }
            else
            {
                dup2(fd0, STDIN_FILENO);
                close(fd0);
            }
        }

        // check if output file redirection has been specified
        if(out)
        {
            // open file for writing to
            fd1 = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            // check fore file open errrors
            if(fd1 < 0)
            {//print nothing because process is in background
                exit(1);
            }
            else
            {
                dup2(fd1, STDIN_FILENO);
                close(fd1);
            }
        }
        else
        {// output file has not been specified so
            // redirect standard out to /dev/null
            fd1 = open("/dev/null", O_WRONLY);
            //check for file open errors
            if(fd1 < 0)
            {
                exit(1);
            }
            else
            {
                dup2(fd1, STDOUT_FILENO);
                close(fd1);
            }
        }
        
        sigintBool = 1;
        // execute the command argument list, if errors, end process
        if(execvp(args[0], args) < 0)
        {
            sigintBool = 0;
            exit(1);
        }
        fflush(stdout);

    }
    else
    { // in the parent
        sigintBool = 0;
        printf("background pid is %d\n", spawnPid);
    }

    //copy the pid into the array
    pidArr[pidPos] = spawnPid;
    pidPos++;

}

             
/*********************************************************************
** Description: processCheck looks at each of the pid's of each child
** saved to the pid array to check if the child process has fini
*********************************************************************/
void processCheck()
{
    int i;
    pid_t result;
    
    // if any process has finished running
    for(i = 0; i < pidPos; i++)
    {
        //get process int returned by waitpid for
        //the process id
        result = waitpid(pidArr[i], &status, WNOHANG);
        if(result > 0)
        {   //if process has finished
            //print information describing how the process
            //exited or was terminated
            printf("background pid %d is done: ", pidArr[i]);
            fflush(stdout);
            if(WIFEXITED(status))
            {
                printf("exit value %d\n", WEXITSTATUS(status));
            }
            else
            {
                printf("terminated by signal %d\n", status);
            }
            fflush(stdout);
            //
            // next we remove the pid from the array
            // at the position it is stored, i
            removePID(i);
        }
    }
    
}

/*********************************************************************
** Description: removePID shifts every value greater than the value at 
** at the position over to take the place of a PID that needs to be 
** removed next
*********************************************************************/
void removePID(int pos)
{
    int i;
    
    //shift all remaining pid values if any
    for(i = pos; i < pidPos - 1; i++)
    {
        //take the place of the prior arry index value
        pidArr[i] = pidArr[i + 1];
    }
    
    //decrement pidPos because we just removed an array
    pidPos--;
}

/*********************************************************************
** Description: freeUpMyArgs frees up the space allocated for arguments 
** in the argumernts array
********************************************************************/
void freeUpMyArgs(char* args[])
{
    int i;
    
    // run through the list where each position
    // has space allocated to hold a string
    for(i = 0; args[i] != NULL; i++)
    {
        // free up that space
        free(args[i]);
        // set to NULL
        args[i] = NULL;
    }
}

/*********************************************************************
** Description: hasBuiltInCmnd checks if the arguments is a built in 
** command and then returns 1 if it is
********************************************************************/
int hasBuiltInCmnd(char* args[])
{
    int i;
    // Check to see if the first argument is a built in
    for (i = 0; i < 3; i++)
    {
        if (strcmp(args[0], builtinStrings[i]) == 0) {
            return 1;
        }
    }
    return 0;
}


/*********************************************************************
** Description: runBuiltInCmnd finds the built in argument and exec
** -utes the proper instructions for the built in argument.
********************************************************************/
void runBuiltInCmnd(char* args[])
{
    int i;
    char *dir; 
    // find which built in command was entered by comparing
    // with our array of built in commands
    for (i = 0; i < 3; i++)
    {
        if (strcmp(args[0], builtinStrings[i]) == 0)
        {
            if(i == 0) // exit command was entered
            {
                keepGoingBool = 0; 
            }
            else if(i == 1) // cd was aentered
            {
                if(args[1] == NULL) // no argument with cd
                {
                    chdir(getenv("HOME")); // cd to home
                    //dir = getCurrentDir();
                    //printf("The current Working Directory is: %s\n", dir);
                    //free(dir);
                    
                }
                else// argument specified
                {
                    // change directory of argument
                    chdir(args[1]);
                    //dir = getCurrentDir();
                    //printf("The current Working Directory is: %s\n", dir);
                    //
                    //free(dir);
                }
            }
            else
            { //status command entered
                if(WIFEXITED(status))
                {
                    printf("exit value %d\n", WEXITSTATUS(status));
                }
                else
                {
                    printf("terminated by signal %d\n", status);
                }
                fflush(stdout);
            }
        }
    }
}





/*********************************************************************
** Description: catches ctrl-z, blocks or unblocks ability to run
** processes in the backgroun
********************************************************************/
void catchSIGTSTP(int signo)
{
    //when background processes are not blocked block them,
    // by changing the global boolean value
    if(backGroundNotBlocked)
    {
        //print message
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(2, message, 50);
        backGroundNotBlocked = 0;
    }
    else
    {
        // when background processes are already blocked, unblock them by
        // setting the global boolean value to 1
        char* message = "\nExiting foreground-only mode\n";
        write(2, message, 30);
        backGroundNotBlocked = 1;
    }
}

