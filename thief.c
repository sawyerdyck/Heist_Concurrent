#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include "helpers.h"
#include <semaphore.h>


/**
 * @brief Initialize a thief
 *
 * Assigns thief id, selects a random thief type,
 * places it into a random non van room, initializes its mutex, and logs the
 * initialization
 *
 * @param[out] thief pointer to the thief struct.
 * @param[in,out] museum pointer to the museum.
 */
void thief_init(struct Thief* thief, struct Museum* museum){
    thief->id = DEFAULT_THIEF_ID;

    const enum ThiefProfile* types;
    int typeNum = get_all_thief_profiles(&types);
    int gType = rand_int_threadsafe(0, typeNum);
    thief->type = types[gType];

    thief->boredom = 0;
    thief->active = true;

    sem_init(&thief->mutex, 0, 1);

    int startRoom = rand_int_threadsafe(1, museum->room_count);
    thief->currentRoom = &museum->rooms[startRoom];

    sem_wait(&thief->currentRoom->mutex);
    thief->currentRoom->thief = thief;
    log_thief_init(thief->id, thief->currentRoom->name, thief->type);
    sem_post(&thief->currentRoom->mutex);
}

/**
 * @brief Move thief to a random connected room
 *
 * thief only moves if no hunters are currently
 * in the room, random connected room is chosen, the thief is safelymoved,
 * and the movement is logged.
 *
 * @param[in,out] thief pointer to the thief moving
 */
void thief_move(struct Thief* thief){
    if(!thief->active){
        return;
    }

    struct Room* thisRoom = thief->currentRoom;
    struct Room* nextRoom = NULL;

    sem_wait(&thisRoom->mutex);
    int huntersPresent = thisRoom->guardCount;
    int connections = thisRoom->connections;

    if(connections == 0){
        sem_post(&thisRoom->mutex);
        return;
    }
    //choose a random room to move to
    int next = rand_int_threadsafe(0, thisRoom->connections);
    nextRoom = thisRoom->connectedRooms[next];
    sem_post(&thisRoom->mutex);

    if(huntersPresent > 0){
        return;
    }

    lock_rooms(thisRoom, nextRoom);

    if(thisRoom->thief == thief){
        thisRoom->thief = NULL;
    }

    thief->currentRoom = nextRoom;
    nextRoom->thief = thief;

    sem_wait(&thief->mutex);
    int boredom = thief->boredom;
    log_thief_move(thief->id, boredom, thisRoom->name, nextRoom->name);
    sem_post(&thief->mutex);

    unlock_rooms(thisRoom, nextRoom);
}

/**
 * @brief thief drops one piece of evidence in its room.
 *
 * selects one of thief evidence bits at random,
 * sets corresponding bit in rooms evidence mask, and logs the
 * haunting.
 *
 * @param[in,out] thief pointer to the thief haunting
 */
void thief_haunt(struct Thief* thief){
    struct Room* room = thief->currentRoom;

    sem_wait(&room->mutex);
    //get thief type
    EvidenceByte drops = thief->type;
    EvidenceByte evidence[3];
    int toDrop = 0;
    //set evidence with thiefs evidence
    for (int i = 0; i < 8; i++) {
        EvidenceByte bit = 1 << i;
        if (drops & bit) {
            evidence[toDrop++] = bit;
        }
    }
    //choose one to drop
    int rand = rand_int_threadsafe(0, 3);
    EvidenceByte drop = evidence[rand];

    room->evidence |= drop;

    sem_wait(&thief->mutex);
    log_thief_evidence(thief->id, thief->boredom, thief->currentRoom->name, drop);
    sem_post(&thief->mutex);

    sem_post(&room->mutex);
}

/**
 * @brief update thiefs state and choose an action.
 *
 * sets boredom based on hunter presence. If boredom reaches max,
 * thief leaves the museum. Otherwise thief chooses to idle, haunt,
 * or move
 *
 * @param[in,out] thief pointer to the thief
 */
void thief_update(struct Thief* thief) {
    if(!thief->active){
        return;
    }

    struct Room* room = thief->currentRoom;

    sem_wait(&room->mutex);
    int huntersPresent = room->guardCount;
    //check for hunters
    sem_wait(&thief->mutex);
    if (huntersPresent > 0) {
        thief->boredom = 0;
    } else {
        thief->boredom++;
    }

    int boredom = thief->boredom;
    //checking boredom
    if(boredom >= ENTITY_BOREDOM_MAX){
        thief->active = false;
        log_thief_exit(thief->id, boredom, room->name);
        sem_post(&thief->mutex);
        sem_post(&room->mutex);
        return;
    }

    int action;
    //if hunters present thief doesnt move
    if(huntersPresent > 0){
        action = rand_int_threadsafe(0, 2);
    } else {
        action = rand_int_threadsafe(0, 3);
    }

    sem_post(&room->mutex);
    //perform whatever action
    if (action == 0) {
        log_thief_idle(thief->id, boredom, thief->currentRoom->name);
        sem_post(&thief->mutex);
    }
    else if (action == 1) {
        sem_post(&thief->mutex);
        thief_haunt(thief);
    }
    else if (action == 2) {
        sem_post(&thief->mutex);
        thief_move(thief);
    }
}

