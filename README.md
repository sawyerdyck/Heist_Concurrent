Final Project : Multi-Threaded Heist Simulation

This program aims to simulate a museum heist where a team of guards explore a museum with a thief present. The thief steals goods and drops evidence while trying to avoid the guards and bore them into leaving. The simulation uses threads and semaphores to coordinate actions safely between the thief and guards. Each guard explores rooms, collects evidence, reacts to stress and boredom, and attempts to return safely. The thief moves around the museum, sometimes dropping evidence, sometimes idling, and sometimes moving. The simulation ends when every guard has left the museum, either by solving the case, reaching maximum stress, or getting bored.

Building and Running
1. Open a terminal and navigate to the project directory.
2. Build the project by running: "make"
3. An executable named p1 will appear in the directory.
4. Run the program using: "make run"
5. To test memory management, run again using "valgrind ./p1"
6. To test for race conditions, recompile with "gcc -Wall -fsanitize=thread *.c -o p1" and run again using "./p1"
7. To clean directory, run: "make clean"

Purpose of Each File

main.c
Runs the simulation, creates threads for all guards and the thief, and handles cleanup when the simulation ends.

museum.c
Manages the musuem structure including contained rooms, the casefile, the dynamic guard list and destruction of all dyanmic data.

room.c
Initializes rooms, connects rooms, adds and removes hunters from rooms, manages room mutexes, and contains the in_van function that controls special behaviour for whenguards are in the start room.

guard.c
Contains full guard behaviour control: movement, breadcrumb tracking, evidence searching, stress and boredom, device swapping, returning, and logging.

thief.c
Contains full thief behaviour control: movement, evidence dropping, boredom tracking, logging actions. The thief stops participating once boredom exceeds maximum.

helpers.c / helpers.h
Contains shared helper functions used by multiple parts of the simulation: logging helpers, random numbers, and evidence helpers.

defs.h
Defines all global constants, enums, structures, evidence bit masks, and shared constants for the project.

path.c 
gives stack operations used to track each hunter breadcrumb trail so they can retrace their steps when returning to the van.

Makefile
Compiles only necessary files to build project.

Bonus Features Included
documentation
improved behaviour - hunters swap devices more intelligently


Sources 
COMP2401 (Carleton University) Course notes and lecture slides on threading, semaphores, helpers.c was partially developed by course instructors to save time.

Developed Individually by Sawyer Dyck 
