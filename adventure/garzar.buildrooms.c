/*********************************************************************
** Program name: Buildrooms
** Author: Romano Garza
** Date: 10/24/2017
** Description: This is the main file for a program that builds a 
** directory of "room" files that contain the name of the room, the 
** connections of the room and the type of room. Used with an adventure
** program.
*********************************************************************/
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
typedef struct Room Room;
typedef struct RoomArr RoomArr;

struct Room
{
    // each room has a name, 6 possible rooms to be connected to
    // and some number of connections
    char name[256];
    int connects[6];
    int numConnects;

};

struct RoomArr
{
    Room* room;
    int size;
    int capacity;
};

//possible room names
char* roomNames[10] =
{
    "Kitchen", "Bathroom", "Foyer", "Field", "Barn", "Roof",
    "Car", "Basement", "Wishing_Well", "Coffin"
};

RoomArr* newRoomArr(int arraySize);
void initRoomArr(RoomArr* r, int arraySize);
void setUpGameBoard(RoomArr* roomArray);
void createFiles(RoomArr* r);
void freeRoomArr(RoomArr* r);
void deleteRoomArr(RoomArr* r);
int sizeRoomArr(RoomArr* r);
int containsRoom(RoomArr* r, char* name);
int allRoomsConnected(RoomArr* r);
int alreadyConnected(Room a, int b);


int main()
{
    // seed random number generator
    srand((int)time(NULL));
    
    //inititialize gameboard array of rooms
    RoomArr* gameBoard;
    int size = 7;
    gameBoard = newRoomArr(size);
    
    // generate game board
    setUpGameBoard(gameBoard);
    
    // create files
    createFiles(gameBoard);

    // delete game board
    deleteRoomArr(gameBoard);
    
    return 0;
}


/*********************************************************************
** Description: Allocate space for and initialize a new Room array
*********************************************************************/
RoomArr* newRoomArr(int arraySize)
{
    assert( arraySize > 0);
    RoomArr* r = (RoomArr *)malloc(sizeof(RoomArr));
    assert(r!=0);
    
    //initializes room array pointer
    initRoomArr(r, arraySize);
    
    //return the room array
    return r;
    
}





/*********************************************************************
** Description: Initialize (including allocation of Room array) dynamic 
** array.
*********************************************************************/
void initRoomArr(RoomArr* r, int arraySize)
{
    assert(arraySize > 0);
    assert(r!=0);
    r->room = (Room *) malloc(sizeof(Room) * arraySize);
    assert(r->room!=0);
    r->size = 0;
    r->capacity = arraySize;
}


/*********************************************************************
** Description: Adds rooms to an initialized room array according to 
** game rules, i.e. every room must have at least 3 connections, can't
** connect to itself, if room a connects to room b, room b must connect
** to room a.
*********************************************************************/
void setUpGameBoard(RoomArr* r)
{
    assert(r!=0);
    int addedRooms[7], count=0, roomNum, roomA, roomB;
    int i = 0, j = 0;
    
    //while not enough rooms have been chosen
    while(sizeRoomArr(r)!=7)
    {
        // choose a random number between 0-9 and add that room name
        // to the room array
        roomNum = (rand()%10);
        
        // keep choosing a different random number if that room has
        // already been added
        while(containsRoom(r, roomNames[roomNum]))
        {
            roomNum = (rand()%10);
        }
        
        // add room name for array of rooms
        memset(r->room[r->size].name, '\0', sizeof(r->room[r->size].name));
        strcpy(r->room[r->size++].name, roomNames[roomNum]);
        
        // add room name number to addedRooms array
        // this will be used later to make connections
        addedRooms[count] = roomNum;
        count++;
    }
    
    // initiallize connections array for each room with flags
    for(i = 0; i < sizeRoomArr(r); i++)
    {
        r->room[i].numConnects = 0;
        for(j = 0; j < 7; j++)
        {
            r->room[i].connects[j] = -1;
        }
        
    }
    
    // while board is not fully connected
    // (board is connected when all rooms have at least 3 connections)
    while(!allRoomsConnected(r))
    {
        // generate random number between 0-6
        // generate random number between 0-6 that is not this number and not already connected
        roomA = (rand()%7);
        roomB = (rand()%7);
        
        while((roomA == roomB) || (alreadyConnected(r->room[roomA], roomB)))
        {
            roomB = (rand()%7);
            roomA = (rand()%7);
        }
        
        // connect rooms by adding each others room array number to
        // the connects array, also incrementing the number of connections
        r->room[roomA].connects[r->room[roomA].numConnects++] = roomB;
        r->room[roomB].connects[r->room[roomB].numConnects++] = roomA;
    }
}


/*********************************************************************
** Description: returns the size of the room array
*********************************************************************/
int sizeRoomArr(RoomArr *r)
{
    assert(r!=0);
    return r->size;
}


/*********************************************************************
** Description: creates files for room array
*********************************************************************/
void createFiles(RoomArr* r)
{
    //get process ID;
    int pID = getpid();
    char directoryS[256] = {};
    memset(directoryS, '\0', sizeof(directoryS));
    sprintf(directoryS, "garzar.rooms.%d", pID);
    //create the directory
    mkdir(directoryS, 0770);
    
    chdir(directoryS);
    //add file data into each file
    int i, j;
    
    for(i = 0; i < sizeRoomArr(r); i++)
    {

        //char* name = malloc(bufferLength * sizeof(char));
        char name[256] = {};
        memset(name, '\0', sizeof(name));
        sprintf(name, "garzar.%s.%d", r->room[i].name, i);
        FILE *currentFile = fopen(name, "w+");
        
        // Add data about room
        // like the name
        fprintf(currentFile, "ROOM NAME: %s", r->room[i].name);
        // the connections
        for(j = 0; j < r->room[i].numConnects; j++)
        {
            fprintf(currentFile, "\nCONNECTION %d: %s", j + 1, r->room[r->room[i].connects[j]].name);
        }
        
        // lastly, the room type
        if(i == 0)
        {
            fprintf(currentFile,"\nROOM TYPE: %s", "START_ROOM");
        }
        else if(i == sizeRoomArr(r)-1)
        {
            fprintf(currentFile,"\nROOM TYPE: %s", "END_ROOM");
        }
        else
        {
            fprintf(currentFile,"\nROOM TYPE: %s", "MID_ROOM");
        }
        
        fclose(currentFile);

    }
    // Return to the original directory.
    chdir("..");
    
}

/*********************************************************************
** Description: checks whether a room has already been added to the 
** room array, returns 1 when it has, 0 otherwise.
*********************************************************************/
int containsRoom(RoomArr*r, char* name)
{
    // check preconditions
    assert(r!=0);
    int i = 0;
    if(sizeRoomArr(r) == 0)
        return 0;
    
    for(i = 0; i < sizeRoomArr(r); i++)
    {
        //check for matching name
        if(strcmp(r->room[i].name, name) == 0)
            return 1;
    }
    return 0;
}


/*********************************************************************
** Description: returns 1 when all rooms are connected, 0 when at
** least 1 is not
*********************************************************************/
int allRoomsConnected(RoomArr*r)
{
    // check preconditions
    assert(r!=0);
    if(sizeRoomArr(r) == 0)
        return 0;
    int i;
    for(i = 0; i < sizeRoomArr(r); i++)
    {
        //check number of room connects
        if(r->room[i].numConnects < 3)
            return 0;
    }
    
    return 1;
}

/*********************************************************************
** Description: returns 1 if room is already connected
*********************************************************************/
int alreadyConnected(Room a, int b)
{
    if(a.numConnects == 0)
    {
        return 0;
    }
    int i;
    for(i = 0; i < a.numConnects ; i++)
    {
        //check for room number already in array
        if( a.connects[i] == b)
            return 1;
    }
    
    return 0;
}


/*********************************************************************
** Description: deallocates each room in the room array
*********************************************************************/
void freeRoomArr(RoomArr *r)
{
    if(r->room != 0)
    {
        free(r->room); 	/* free the space on the heap */
        r->room = 0;   	/* make it point to null */
    }
    r->size = 0;
    r->capacity = 0;
}



/*********************************************************************
** Description: makes call to deallocate rooms, finally deallocates the
** room array
*********************************************************************/
void deleteRoomArr(RoomArr *r)
{
    freeRoomArr(r);
    free(r);
}
