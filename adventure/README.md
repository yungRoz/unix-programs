# Adventure

These are a couple C programs I wrote to demonstrate familiarity with C programming on UNIX and common file read and write operations.

## Overview

It's a game! One of the programs generates a bunch of room files and possible connections to other rooms. The other program enables a player to navigate these files and try to find the "END" room. Super fun! :)

### Cool stuff

Mutex-based time keeping via read/write operations and multithreads by the pthread library.

### Run

First compile the programs! Run the garzar.buildrooms program to generate rooms and connections. Run the garzar.adventure program to begin the game. Move to different rooms and try to find the end.

```
$ gcc -o garzar.buildrooms garzar.buildrooms.c
$ ./garzar.buildrooms
$ gcc -o garzar.adventure garzar.adventure.c -lpthread
$ ./garzar.adventure
CURRENT LOCATION: Kitchen
POSSIBLE CONNECTIONS: Bathroom, Foyer, Coffin.
WHERE TO? >
```

### Example Gameplay

```
$ ./garzar.adventure
CURRENT LOCATION: Barn
POSSIBLE CONNECTIONS:  Bathroom,  Coffin,  Wishing_Well,  Foyer,  Field,  Roof.
WHERE TO? >Bathroom


CURRENT LOCATION: Bathroom
POSSIBLE CONNECTIONS:  Barn,  Foyer,  Barn,  Roof,  Coffin,  Field.
WHERE TO? >foyer


HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.

WHERE TO? >time

03:34pm Friday, July 27, 2018
WHERE TO? >Coffin


CURRENT LOCATION: Coffin
POSSIBLE CONNECTIONS:  Barn,  Field,  Wishing_Well.
WHERE TO? >Wishing_Well


YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!
YOU TOOK 4 STEPS. YOUR PATH TO VICTORY WAS:
Barn
Bathroom
Coffin
Wishing_Well
```

### Instructors Original Specifications

The first thing your rooms program must do is generate 7 different room files, one room per file, in a directory called "<STUDENT ONID USERNAME>.rooms.<PROCESS ID>". You get to pick the names for those files, which should be hard-coded into your program.

Each room has a Room Name, at least 3 outgoing connections (and at most 6 outgoing connections, where the number of outgoing connections is random) from this room to other rooms, and a room type. The connections from one room to the others should be randomly assigned – i.e. which rooms connect to each other one is random - but note that if room A connects to room B, then room B must have a connection back to room A. Because of these specs, there will always be at least one path through. Note that a room cannot connect to itself, nor can a room connect to another room more than once: i.e. room A cannot have more than one outbound connection to room B.

Choose a list of ten different Room Names, hard coded into your rooms program, and have your rooms program randomly assign a room name to each room generated. For a given run of your program, 7 of the 10 room names will be used. Note that a room name cannot be used to in more than one room,

The possible room type entries are: START_ROOM, END_ROOM, and MID_ROOM. The assignment of which room gets which type should be random. Naturally, only one room should be assigned as the start room, and only one room should be assigned as the end room.

The ordering of the connections from a room to the other rooms, in the file, does not matter. Note that the randomization you do here to define the layout is not all that important: just make sure the connections between rooms, the room names themselves and which room is which type, is somewhat different each time, however you want to do that. We're not evaluating your randomization procedure, though it's not acceptable to just randomize the room names but use the same room structure every time.

I highly recommend building up the room graph in this manner: adding connections two at a time (forwards and backwards), to randomly chosen room endpoints, until the graph satisfies the requirements. It's easy, requires no backtracking, and tends to generate sparser layouts. As a warning, the method of choosing the number of connections beforehand that each room will have is not recommended, as it's hard to make those chosen numbers match the constraints of the graph. To help do this correctly, please read the article 2.2 Program Outlining in Program 2 and consider using the room-generating pseudo-code listed!

Now let’s describe what should be presented to the player in the game. Upon being executed, after the rooms program has run and the rooms are generated, the game should present an interface to the player. Note that the room data must be read back into the program from the previously-generated room files, for use by the game. Since the rooms program may have been run multiple times before executing the game, your game should use the most recently created files: perform a stat() function call (Links to an external site.)Links to an external site. on rooms directories in the same directory as the game, and open the one with most recent st_mtime component of the returned stat struct.

This player interface should list where the player current is, and list the possible connections that can be followed. It should also then have a prompt.

If the user types anything but a valid room name from this location (case matters!), the game should return an error line that says “HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.”, and repeat the current location and prompt.

Trying to go to an incorrect location does not increment the path history or the step count. Once the user has reached the End Room, the game should indicate that it has been reached. It should also print out the path the user has taken to get there, the number of steps taken (not the number of rooms visited, which would be one higher because of the start room), a congratulatory message, and then exit.

Note the punctuation used in the complete example below: we're looking for the same punctuation in your program.

When your program exits, set the exit status code to 0, and leave the rooms directory in place, so that it can be examined.

If you need to use temporary files (you probably won't), place them in the directory you create, above. Do not leave any behind once your program is finished. We will not test for early termination of your program, so you don’t need to watch for those signals.

Do not use the -C99 standard or flag when compiling - this should be done using raw C.

Your game program must also be able to return the current time of day by utilizing a second thread and mutex(es). I recommend you complete all other portions of this assignment first, then add this mutex-based timekeeping component last of all. Use the classic pthread library for this simple multithreading, which will require you to use the "-lpthread" compile option switch with gcc (see below for compilation example).

When the player types in the command "time" at the prompt, and hits enter, a second thread must write the current time (in the format shown below) to a file called "currentTime.txt", which should be located in the same directory as the game. The main thread will then read this time value from the file and print it out to the user, with the next prompt on the next line. I recommend you keep the second thread running during the execution of the main program, and merely wake it up as needed via this "time" command. In any event, at least one mutex must be used to control execution between these two threads.
