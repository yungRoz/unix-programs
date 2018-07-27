/*********************************************************************
** Program name: Adventure
** Author: Romano Garza
** Date: 10/24/2017
** Description: This is the main file for an adventure program that
** allows the user to traverse rooms (files) in search of the end room
** To traverse rooms, the user inputs the connection room they wish to
** enter.
*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

//FUNCTION PROTOTYPES
void startGame(char dir[]);
void getDirName(char target[], char dir[]);
void getDataFromFile(char dir[]);
void printData();
void timeThread();
void readTime();
void* writeTime();
int isNotConnection(char userInput[]);

//GLOBAL VARIABLES
char currRoom[100];         //holds the current room the player is in
char newestRoomDir[100];    //holds the newest "garzar.rooms..." directory
char connections[6][100];   //holds the current rooms connections
char path[40][100];         //holds the rooms traversed by the player
int connectionsAdded = 0;   //index and counter for room connections array
int steps = 1;              //holds the steps the user has taken
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

int main()
{
    //this is the prefix of the directory we're looking for
    char targetDirPrefix[32] = "garzar.rooms.";

    //using the prefix and the newestRoomDir global variab
    //get newest rooms directory
    getDirName(targetDirPrefix, newestRoomDir);

    //start game
    startGame(newestRoomDir);
    return 0;


}


/*********************************************************************
** Description: This function takes the directory of all the room files
** as an argument. Then it saves a string for the starting room and end
** room. While the player hasn't entered the end room, the file of the
** room the player wishes to enter (while valid) is opened, and the
** name and connection of said room is displayed. Finally, upon entering
** the end_room the player receives a congratulatory message.
*********************************************************************/
void startGame(char dir[])
{
    chdir(dir);
    //prefix of start_room file's name
    char targetStart[2] = "0";
    //prefix of end_room file's name
    char targetFinish[2] = "6";
    //prefix for string of user input
    char targetWhereTo[256];

    //strings for holding file names
    char endFileName[100];
    char currentFileName[100];

    //get the file names of both the start and end room file names
    getDirName(targetStart, currentFileName);
    getDirName(targetFinish, endFileName);

    //loop while player hasn't entered the end room
    while(strstr(currentFileName, endFileName) == NULL)
    {
        //open file, get data, save data, close file
        getDataFromFile(currentFileName);
        //store current roomName
        strcpy(path[steps], currRoom);

        //display data
        printData();

        //get next move
        printf("WHERE TO? >");
        scanf("%99s", targetWhereTo);
        printf("\n\n");

        //while user has not entered a valid connection
        while(isNotConnection(targetWhereTo))
        {
            if(strcmp(targetWhereTo, "time") == 0)
            {
                //move out of newest rooms directory
                chdir("..");
                //start time thread
                timeThread();
                //read time
                readTime();
                //move back into newest rooms directory
                chdir(dir);
            }
            else
            {
                printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
                printData();
            }
            printf("WHERE TO? >");
            scanf("%99s", targetWhereTo);
            printf("\n\n");

        }


        //find file, store in currentFileName
        getDirName(targetWhereTo, currentFileName);
        //inc steps
        steps++;
    }
    //save the end_room name as last step in path
    strcpy(path[steps], targetWhereTo);
    //read out victory message
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS: \n", steps);
    //display path
    int i;
    for(i = 1; i < steps + 1; i++)
    {
        printf("%s", path[i]);
    }
    printf("\n");


}

/*********************************************************************
** Description: Adapted from the instructors notes, finds the newest
** file with the target string, stores it in the dir string passed
*********************************************************************/
void getDirName(char target[], char dir[])
{
    //get newest directory with target prefix
    int  newestDirTime= -1; // Modified timestamp of newest subdir examined
    DIR* dirToCheck; // Holds the directory we're starting in
    struct dirent *fileInDir; // Holds the current subdir of the starting dir
    struct stat dirAttributes; // Holds information we've gained about subdir

    dirToCheck = opendir("."); // Open up the directory this program was run in

    if (dirToCheck > 0) // Make sure the current directory could be opened
    {
        while ((fileInDir = readdir(dirToCheck)) != NULL) // Check each entry in dir
        {
            if (strstr(fileInDir->d_name, target) != NULL) // If entry has prefix
            {
                stat(fileInDir->d_name, &dirAttributes); // Get attributes of the entry

                if ((int)dirAttributes.st_mtime > newestDirTime) // If this time is bigger
                {
                    newestDirTime = (int)dirAttributes.st_mtime;
                    memset(dir, '\0', sizeof(dir));
                    strcpy(dir, fileInDir->d_name);
                }
            }
        }
    }
    closedir(dirToCheck); // Close the directory we opened
}

/*********************************************************************
** Description: Takes a string of the room file to open, opens it,
** reads room data into appropriate global variables.
*********************************************************************/
void getDataFromFile(char dir[])
{
    //get data from file
    FILE *file = fopen (dir, "r" );

    // reset connectionsAdded
    connectionsAdded = 0;

    //check that file could be opened
    if (file != NULL) {

        //string buffer for line that's read in
        char line [256];

        //get first line of file, which contains current roomname
        fgets(line, sizeof line, file);
        //string gets tokenized appropriately
        char* token = strtok(line, " :");
        token = strtok(NULL, " :");
        token = strtok(NULL, " :");
        //save currRoom
        strcpy(currRoom, token);

        //line by line, get connections and save them
        while(fgets(line, sizeof line, file)!= NULL)
        {
            if(line[0] == 'C')
            {
                char* token = strtok(line, " :");
                token = strtok(NULL, " :");
                token = strtok(NULL, " :");

                //get rid of el new line characterito
                size_t n = strlen( token );
                if ( n && token[n-1] == '\n' ) token[n-1] = '\0';

                //copy the room connection string (token) into the array of
                //connections
                strcpy(connections[connectionsAdded++], token);
            }
        }
        fclose(file);
    }
    else{
        printf("ERROR OPENING ROOM FILE!!");
    }

}


/*********************************************************************
** Description: prints data for current room the player is in
*********************************************************************/
void printData()
{

    printf("CURRENT LOCATION: %s", currRoom);
    printf("POSSIBLE CONNECTIONS: ");
    int i = 0;
    for(i = 0; i < connectionsAdded; i ++ )
    {
        if(i!=connectionsAdded - 1)
        {
            printf(" %s, ", connections[i]);
        }
        else
        {
            printf(" %s.\n", connections[i]);
        }
    }
}

/*********************************************************************
** Description: takes user input and checks if it is not a valid
** connected room.
*********************************************************************/
int isNotConnection(char userInput[])
{
    int i;
    for(i = 0; i < connectionsAdded; i ++ )
    {
        if(strcmp(userInput, connections[i]) == 0)
        {
            return 0;
        }
    }
    return 1;
}

/*********************************************************************
** Description: starts a second thread to write the current time to a
** currentTime.txt file
*********************************************************************/
void timeThread()
{
    pthread_t secondT;
    //lock current thread
    pthread_mutex_lock(&myMutex);

    //start running second thread
    pthread_create(&secondT, NULL, writeTime, NULL);

    //unlock current thread
    pthread_mutex_unlock(&myMutex);
    pthread_join(secondT, NULL);
}


/*********************************************************************
** Description: writes the current time to a currentTime.txt file
*********************************************************************/
void* writeTime()
{
    //open file for writing
    FILE* file = fopen("currentTime.txt", "w");

    //string to hold time
    char timeS[300];
    //safely initialize with nulls
    memset(timeS, '\0',sizeof(timeS));

    //time variable
    time_t timeNow;
    //get time, assign to timeNow
    time(&timeNow);

    //struct for formatting time data
    struct tm* timeData;
    //format
    timeData = localtime(&timeNow);
    //further format into the time string
    strftime(timeS, 300, "%I:%M%P %A, %B %d, %Y", timeData);
    //write to file
    fprintf(file,"%s",timeS);
    fclose(file);
    return NULL;
}

/*********************************************************************
** Description: starts a second thread to write the current time to a
** currentTime.txt file
*********************************************************************/
void readTime()
{
    FILE* file = fopen("currentTime.txt", "r");
    //string buffer for line that's read in
    char line[256];

    //get first line of file, which contains the time
    fgets(line, sizeof line, file);
    //display
    printf("%s\n", line);
    //close file
    fclose(file);
}
