#include "defs.h"
#include "helpers.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief initialize guard and place them in the van
 *
 * copies guard name, assigns id, choose random evidence device,
 * initialize all fields and the breadcrumb stack, and
 * inserts the guard into the van. logs the initialization
 *
 * @param[out] guard Pointer to the Guard being initialized.
 * @param[in,out] museum Pointer to the Museum the guard belongs to.
 * @param[in] name Display name assigned to the guard.
 * @param[in] id Unique guard identifier.
 */
void guard_init(struct Guard* guard, struct Museum* museum, const char* name, int id){
    strncpy(guard->name, name, MAX_GUARD_NAME);
    guard->name[MAX_GUARD_NAME - 1] = '\0';

    guard->id = id;
    guard->currentRoom = museum->starting_room;

    const enum TamperType* evidence;
    int evNum = get_all_tamper_types(&evidence);
    guard->device = evidence[rand_int_threadsafe(0, evNum)];

    guard->casefile = &museum->casefile;
    guard->stress = 0;
    guard->boredom = 0;

    roomstack_init(&guard->breadcrumb);

    guard->active = true;
    guard->inControlRoom = true;
    guard->returningToControl = false;
    guard->starting = true;
    guard->whyExit = LR_CLUES;

    sem_init(&guard->mutex, 0, 1);

    sem_wait(&museum->starting_room->mutex);
    add_guard(museum->starting_room, guard);
    log_guard_init(guard->id, museum->starting_room->name, guard->name, guard->device);
    sem_post(&museum->starting_room->mutex);
}

/**
 * @brief change guards device to a different type
 *
 * select a new device that is neither identical to the current device nor
 * already collected within the casefile and log the swap
 *
 * @param[in,out] guard pointer to the guard switching devices
 */
void change_device(struct Guard* guard){
    const enum TamperType* evidence;
    int evNum = get_all_tamper_types(&evidence);

    sem_wait(&guard->casefile->mutex);
    EvidenceByte collected = guard->casefile->collected;
    sem_post(&guard->casefile->mutex);

    sem_wait(&guard->mutex);
    enum TamperType curr = guard->device;
    int boredom = guard->boredom;
    int stress = guard->stress;
    sem_post(&guard->mutex);

    int ind = rand_int_threadsafe(0, evNum);

    if (evNum > 1){
        while (evidence[ind] == curr || (collected & evidence[ind]) != 0){
            ind = rand_int_threadsafe(0, evNum);
        }
    }

    enum TamperType new = evidence[ind];

    sem_wait(&guard->mutex);
    guard->device = new;
    sem_post(&guard->mutex);

    log_swap(guard->id, boredom, stress, curr, new);
}

/**
 * @brief move guard one step along breadcrumb path towards van
 *
 * pop one room from the breadcrumb stack transfers the guard into that
 * room and log the movement
 *
 * @param[in,out] guard pointer to guard
 */
void exit_to_control_room(struct Guard* guard){
    struct Room* thisRoom = guard->currentRoom;
    struct Room* nextRoom = pop(&guard->breadcrumb);

    lock_rooms(thisRoom, nextRoom);
    remove_guard(thisRoom, guard);
    add_guard(nextRoom, guard);

    sem_wait(&guard->mutex);
    guard->currentRoom = nextRoom;
    guard->inControlRoom = nextRoom->isExit;
    int boredom = guard->boredom;
    int stress = guard->stress;
    enum TamperType device = guard->device;
    bool isExit = nextRoom->isExit;
    sem_post(&guard->mutex);

    log_move(guard->id, boredom, stress, thisRoom->name, nextRoom->name, device);
    unlock_rooms(thisRoom, nextRoom);

    if (isExit){
    }
}

/**
 * @brief move guard to random connected room
 *
 * if guard is currently returning to van, the function redirects to
 * exit_to_control_room(). otherwise select a random connected room, push the
 * current room onto the breadcrumb stack and log move.
 *
 * @param[in,out] guard pointer to guard moving
**/
void guard_move(struct Guard* guard){
    sem_wait(&guard->mutex);
    bool active = guard->active;
    bool returning = guard->returningToControl;
    sem_post(&guard->mutex);

    if (!active){
        return;
    }
    if (returning){
        exit_to_control_room(guard);
        return;
    }

    struct Room* thisRoom = guard->currentRoom;
    struct Room* nextRoom = NULL;

    sem_wait(&thisRoom->mutex);
    int connections = thisRoom->connections;
    int toWhere = rand_int_threadsafe(0, connections);
    nextRoom = thisRoom->connectedRooms[toWhere];
    sem_post(&thisRoom->mutex);

    lock_rooms(thisRoom, nextRoom);

    if (nextRoom->guardCount >= MAX_ROOM_OCCUPANCY){
        unlock_rooms(thisRoom, nextRoom);
        return;
    }

    remove_guard(thisRoom, guard);

    sem_wait(&guard->mutex);
    bool shouldPush = !guard->returningToControl;
    sem_post(&guard->mutex);

    if (shouldPush){
        push(&guard->breadcrumb, thisRoom);
    }

    sem_wait(&guard->mutex);
    guard->currentRoom = nextRoom;
    guard->inControlRoom = nextRoom->isExit;
    int boredom = guard->boredom;
    int stress = guard->stress;
    enum TamperType device = guard->device;
    bool isExit = nextRoom->isExit;
    sem_post(&guard->mutex);

    add_guard(nextRoom, guard);

    log_move(guard->id, boredom, stress, thisRoom->name, nextRoom->name, device);
    unlock_rooms(thisRoom, nextRoom);

    if (isExit){
        sem_wait(&guard->mutex);
        guard->returningToControl = false;
        sem_post(&guard->mutex);
    }
}

/**
 * @brief search current room for evidence and collect it if so
 *
 * if guards device matches evidence present in room the evidence
 * is collected, removed from the room and added to the casefile, and a return
 * to the van is triggered. guards sometime get a "bad feeling";; and go back to the van
 *
 * @param[in,out] guard pointer to guard searching for evidence
 */
void scan_for_clues(struct Guard* guard){
    struct Room* room = guard->currentRoom;
    EvidenceByte inRoom = room->evidence;

    sem_wait(&guard->mutex);
    EvidenceByte device = guard->device;
    int boredom = guard->boredom;
    int stress = guard->stress;
    bool inControlRoom = guard->inControlRoom;
    sem_post(&guard->mutex);

    EvidenceByte match = inRoom & device;

    if (match != 0){
        room->evidence = room->evidence & ~device;

        sem_wait(&guard->casefile->mutex);
        guard->casefile->collected |= device;
        if (evidence_is_valid_ghost(guard->casefile->collected)){
            guard->casefile->solved = true;
        }
        sem_post(&guard->casefile->mutex);

        log_evidence(guard->id, boredom, stress, room->name, device);
        if (!inControlRoom){
            log_return_to_van(guard->id, boredom, stress, guard->currentRoom->name, device, true);

            sem_wait(&guard->mutex);
            guard->returningToControl = true;
            sem_post(&guard->mutex);
        }
        return;
    }

    int badFeeling = rand_int_threadsafe(0, 25);

    if (!inControlRoom && badFeeling == 0){
        log_return_to_van(guard->id, boredom, stress, guard->currentRoom->name, device, true);

        sem_wait(&guard->mutex);
        guard->returningToControl = true;
        sem_post(&guard->mutex);
    }
}

/**
 * @brief determine whether guard should exit due to stress or boredom
 *
 * a guard exits if their stress reaches GUARD_STRESS_MAX, or their boredom reaches ENTITY_BOREDOM_MAX.
 *
 * in either case, the guard is removed from room immediately, marked inactive and logged.
 *
 * @param[in,out] guard pointer to guard
 *
 * @return true if the guard exited, false otherwise.
 */
bool consider_exiting(struct Guard* guard){
    struct Room* room = guard->currentRoom;
    sem_wait(&guard->mutex);
    int stress = guard->stress;
    int boredom = guard->boredom;
    enum TamperType device = guard->device;
    sem_post(&guard->mutex);

    if (stress >= GUARD_STRESS_MAX){
        remove_guard(room, guard);

        sem_wait(&guard->mutex);
        guard->active = false;
        guard->whyExit = LR_OVERWHELMED;
        sem_post(&guard->mutex);

        log_exit(guard->id, boredom, stress, room->name, device, LR_OVERWHELMED);
        return true;

    } else if (boredom >= ENTITY_BOREDOM_MAX){
        remove_guard(room, guard);

        sem_wait(&guard->mutex);
        guard->active = false;
        guard->whyExit = LR_BORED;
        sem_post(&guard->mutex);

        log_exit(guard->id, boredom, stress, room->name, device, LR_BORED);
        return true;
    }

    return false;
}

/**
 * @brief update guards boredom and stress based on thief presence
 *
 * increments stress and resets boredom when guard shares a room with
 * a thief, otherwise increments boredom.
 *
 * @param[in,out] guard pointer to guard
 */
void update_state(struct Guard* guard){
    struct Room* room = guard->currentRoom;
    bool isThief = (room->thief != NULL);
    sem_wait(&guard->mutex);
    if (isThief){
        guard->boredom = 0;
        guard->stress++;
    } else {
        guard->boredom++;
    }
    sem_post(&guard->mutex);
}

/**
 * @brief a full guard turn
 *
 * performs update state, check for exit conditions checks if in van and calls iv_van
 * behavior performs evidence searching and moves the guard when allowed
 *
 * @param[in,out] guard pointer to guard taking turn
 */
void guard_take_turn(struct Guard* guard){
    if (!guard->active){
        return;
    }

    struct Room* room = guard->currentRoom;
    bool wasReturning, inControlRoom;
    sem_wait(&guard->mutex);
    wasReturning = guard->returningToControl;
    inControlRoom = guard->inControlRoom;
    sem_post(&guard->mutex);

    if (inControlRoom && wasReturning){
        in_control_room(guard);
        if (!guard->active){
            return;
        }
    }

    sem_wait(&room->mutex);
    update_state(guard);

    if (consider_exiting(guard)){
        sem_post(&room->mutex);
        return;
    }

    sem_wait(&guard->mutex);
    wasReturning = guard->returningToControl;
    sem_post(&guard->mutex);

    if (wasReturning){
        sem_post(&room->mutex);
        exit_to_control_room(guard);
        return;
    }

    scan_for_clues(guard);

    bool nowReturning;
    sem_wait(&guard->mutex);
    nowReturning = guard->returningToControl;
    sem_post(&guard->mutex);
    sem_post(&room->mutex);

    if (nowReturning){
        return;
    }

    guard_move(guard);
}

