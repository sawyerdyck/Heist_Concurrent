OPT = -Wall -g
OBJ = main.o thief.o guard.o helpers.o museum.o room.o path.o 

project: $(OBJ) defs.h 
	gcc $(OPT) $(OBJ) -o p1
main.o: main.c defs.h
	gcc $(OPT) -c main.c
helpers.o: helpers.c helpers.h defs.h
	gcc $(OPT) -c helpers.c
thief.o: thief.c defs.h
	gcc $(OPT) -c thief.c 
guard.o: guard.c defs.h
	gcc $(OPT) -c guard.c
museum.o: museum.c defs.h
	gcc $(OPT) -c museum.c
room.o: room.c defs.h
	gcc $(OPT) -c room.c
path.o: path.c defs.h
	gcc $(OPT) -c path.c
run: p1
	./p1
clean: 
	rm -f *.o *.csv p1 
