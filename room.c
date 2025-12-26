#include "defs.h"
#include <string.h>
#include <semaphore.h>
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief initialize room struct using default values
 *
 * sets name, counters, flags, links, and mutex 
 * 
 * @param[out] room pointer to room being set up
 * @param[in] name char for room
 * @param[in] isExit flag showing if room is the van
 */
void room_init(struct Room* room, const char* name, bool isExit){
    strncpy(room->name, name, MAX_ROOM_NAME);
    room->name[MAX_ROOM_NAME - 1] = '\0';
    room->connections = 0;
    room->thief = NULL;
    room->guardCount = 0;
    room->isExit = isExit;
    room->evidence = 0;

    for (int i = 0; i < MAX_CONNECTIONS; i++){
        room->connectedRooms[i] = NULL;
    }
    for (int i = 0; i < MAX_ROOM_OCCUPANCY; i++){
        room->guards[i] = NULL;
    }

    sem_init(&room->mutex, 0, 1);
}

/**
 * @brief link two rooms so movement between them becomes possible
 *
 * new link added to each room list if room still has open link slot
 *
 * @param[in,out] first pointer to room being linked
 * @param[in,out] second pointer to room being linked
 */
void room_connect(struct Room* first, struct Room* second){
    if (first->connections < MAX_CONNECTIONS){
        first->connectedRooms[first->connections++] = second;
    }
    if (second->connections < MAX_CONNECTIONS){
        second->connectedRooms[second->connections++] = first;
    }
}

/**
 * @brief place hunter inside room
 *
 * hunter count grows if room has open slot
 *
 * @param[in,out] room pointer room receiving hunter
 * @param[in] hunter pointer to hunter being added
 */
void add_guard(struct Room* room, struct Guard* hunter){
    if (room->guardCount >= MAX_ROOM_OCCUPANCY){
        return;
    }

    room->guards[room->guardCount++] = hunter;
}

/**
 * @brief remove hunter from room
 *
 * removes hunter from dynamic array and performs arraylist shift
 *
 * @param[in,out] room pointer to room holding hunter
 * @param[in] hunter pointer to hunter being removed
 */
void remove_guard(struct Room* room, struct Guard* hunter){
    if (!room || !hunter){
        return;
    }

    if (room->guardCount == 0){
        return;
    }

    for (int i = 0; i < room->guardCount; i++){
        if (room->guards[i] == hunter){
            for (int j = i; j < room->guardCount - 1; j++){
                room->guards[j] = room->guards[j + 1];
            }

            room->guards[room->guardCount - 1] = NULL;
            room->guardCount--;
            break;
        }
    }
}

/**
 * @brief lock two rooms using pointer order to prevent deadlock
 *
 * @param[in,out] from pointer to first room
 * @param[in,out] to pointer to second room
 */
void lock_rooms(struct Room* from, struct Room* to){
    if (from < to){
        sem_wait(&from->mutex);
        sem_wait(&to->mutex);
    } else {
        sem_wait(&to->mutex);
        sem_wait(&from->mutex);
    }
}

/**
 * @brief unlock pair of rooms locked earlier by lock_rooms in order
 *
 * @param[in,out] from pointer to first room
 * @param[in,out] to pointer to second room
 */
void unlock_rooms(struct Room* from, struct Room* to){
    if (from < to){
        sem_post(&from->mutex);
        sem_post(&to->mutex);
    } else {
        sem_post(&to->mutex);
        sem_post(&from->mutex);
    }
}

/**
 * @brief run logic for hunter inside van
 *
 * stack cleared, casefile checked, state updated, exit logged
 * if full set of evidence found, hunter leaves sim
 *
 * @param[in,out] hunter pointer to hunter in van
 */
void in_control_room(struct Guard* hunter){
    struct Room* room = hunter->currentRoom;
    if (!room->isExit){
        return;
    }

    empty_roomstack(&hunter->breadcrumb);

    sem_wait(&hunter->casefile->mutex);
    bool solved = hunter->casefile->solved;
    EvidenceByte collected = hunter->casefile->collected;
    sem_post(&hunter->casefile->mutex);

    sem_wait(&hunter->mutex);
    int boredom = hunter->boredom;
    int stress = hunter->stress;
    enum TamperType device = hunter->device;
    bool wasReturning = hunter->returningToControl;
    hunter->returningToControl = false;
    sem_post(&hunter->mutex);

    sem_wait(&room->mutex);
    update_state(hunter);
    sem_post(&room->mutex);

    if (solved && evidence_is_valid_ghost(collected)){
        sem_wait(&room->mutex);
        remove_guard(room, hunter);
        sem_post(&room->mutex);

        sem_wait(&hunter->mutex);
        hunter->active = false;
        hunter->whyExit = LR_CLUES;
        sem_post(&hunter->mutex);

        log_return_to_van(hunter->id, boredom, stress, room->name, device, false);
        log_exit(hunter->id, boredom, stress, room->name, device, LR_CLUES);
        return;
    }

    if (wasReturning){
        log_return_to_van(hunter->id, boredom, stress, room->name, device, false);
        change_device(hunter);
    }
}

