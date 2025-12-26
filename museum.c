#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief initialize casefile
 *
 * sets collected evidence to zero, marks case as unsolved, initializes mutex
 *
 * @param[out] file pointer to the casefile
 */
void casefile_init(struct CaseFile* file){
    file->collected = 0;
    file->solved = false;
    sem_init(&file->mutex, 0, 1);
}

/**
 * @brief close a casefile.
 *
 * used during cleanup to destroy mutex
 *
 * @param[in,out] file pointer to the casefile
 */
void casefile_close(struct CaseFile* file){
    sem_destroy(&file->mutex);
}

/**
 * @brief initialize museum struct
 *
 * sets room count, guard list capacity, and museum pointers, initializes casefile
 *
 * @param[out] museum pointer to new museum
 */
void museum_init(struct Museum* museum){
    museum->room_count = 0;
    museum->starting_room = NULL;
    museum->guards = NULL;
    museum->guardCount = 0;
    museum->guardMax = 0;
    casefile_init(&museum->casefile);
}

/**
 * @brief free all memory associated with museum.
 *
 * frees every guard, destroys room mutexes, clears casefile,
 * and frees the guard pointer array
 *
 * @param[in,out] museum pointer to museum being cleaned up.
 */
void museum_cleanup(struct Museum* museum){
    for(int i = 0; i < museum->guardCount; i++){
        struct Guard* guard = museum->guards[i];
        if(!guard){
            continue;
        }

        empty_roomstack(&guard->breadcrumb);
        sem_destroy(&guard->mutex);
        free(guard);
    }

    for(int i = 0; i < museum->room_count; i++){
        sem_destroy(&museum->rooms[i].mutex);
    }

    casefile_close(&museum->casefile);

    free(museum->guards);
    museum->guards = NULL;
    museum->guardCount = 0;
    museum->guardMax = 0;
}

/**
 * @brief add a guard to the museum.
 *
 * resizes guard array if needed using arraylist logic, allocates a new guard,
 * initializes it, and stores it in the museum
 *
 * @param[in,out] museum pointer to the museum
 * @param[in] name the guard's name
 * @param[in] id the guard's id
 *
 * @return true if the guard was added successfully
 * @return false if memory allocation failed
 */
bool museum_add_guard(struct Museum* museum, const char* name, int id){
    if(museum->guardCount >= museum->guardMax){
        int resize = (museum->guardMax == 0 ? 1 : 2 * museum->guardMax);

        struct Guard** newArr =
            realloc(museum->guards, resize * sizeof(struct Guard*));

        if(!newArr){
            return false;
        }

        museum->guards = newArr;
        museum->guardMax = resize;
    }

    struct Guard* guard = malloc(sizeof(struct Guard));
    if(!guard){
        return false;
    }

    museum->guards[museum->guardCount] = guard;
    museum->guardCount++;

    guard_init(guard, museum, name, id);

    return true;
}

