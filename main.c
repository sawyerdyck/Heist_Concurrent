#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "helpers.h"
#include <pthread.h>

void* thief_thread(void* arg) {
    struct Thief* thief = arg;
    while (thief->active) {
        thief_update(thief);
    }
    return NULL;
}

void* guard_thread(void* arg) {
    struct Guard* guard = arg;
    while (guard->active) {
        guard_take_turn(guard);
    }
    return NULL;
}

int main() {
    // create museum
    struct Museum museum;
    museum_init(&museum);
    museum_populate_rooms(&museum);

    // get user input
    printf("Enter guard names and IDs (type 'done' when finished):\n");

    while (1) {
        char name[MAX_GUARD_NAME];

        printf("Enter guard name (or 'done' to finish): ");
        if (!fgets(name, sizeof(name), stdin))
            break;

        // strip newline
        char* newL = strchr(name, '\n');
        if (newL) *newL = '\0';

        if (strcmp(name, "done") == 0)
            break;

        // read id safely
        char trimId[32];
        printf("Enter guard ID: ");
        if (!fgets(trimId, sizeof(trimId), stdin))
            break;

        int id = atoi(trimId);

        museum_add_guard(&museum, name, id);
    }

    thief_init(&museum.thief, &museum);

    pthread_t thiefThread;
    pthread_create(&thiefThread, NULL, thief_thread, &museum.thief);

    pthread_t* guardThreads = malloc(sizeof(pthread_t) * museum.guardCount);
    for (int i = 0; i < museum.guardCount; i++) {
        pthread_create(&guardThreads[i], NULL, guard_thread, museum.guards[i]);
    }

    pthread_join(thiefThread, NULL);

    // wait for guards to finish
    for (int i = 0; i < museum.guardCount; i++) {
        pthread_join(guardThreads[i], NULL);
    }

    free(guardThreads);

    // display output
    printf("================================================\n");
    printf("Heist Simulation Results:\n");
    printf("================================================\n");

    for (int i = 0; i < museum.guardCount; i++) {
        struct Guard* guard = museum.guards[i];
        const char* reason = exit_reason_to_string(guard->whyExit);

        printf("[%s] Guard %s (ID %d) exited because of [%s] (bored=%d stress=%d).\n",
               (guard->whyExit == LR_OVERWHELMED) ? "✗" : " ",
               guard->name, guard->id, reason, guard->boredom, guard->stress);
    }

    printf("\nShared Case File Checklist:\n");
    EvidenceByte collected = museum.casefile.collected;

    const enum TamperType* tamperTypes;
    int tamperCount = get_all_tamper_types(&tamperTypes);

    for (int i = 0; i < tamperCount; i++) {
        bool hasIt = (collected & tamperTypes[i]) != 0;
        printf("  - [%s] %s\n", hasIt ? "✓" : " ", tamper_to_string(tamperTypes[i]));
    }

    printf("\nVictory Results:\n");
    printf("----------------------------------------------------\n");

    int guardsWon = 0;
    for (int i = 0; i < museum.guardCount; i++) {
        if (museum.guards[i]->whyExit == LR_CLUES) {
            guardsWon++;
        }
    }

    printf("- Guards exited after identifying the thief: %d/%d\n", guardsWon, museum.guardCount);

    const enum ThiefProfile* thiefProfiles;
    int thiefCount = get_all_thief_profiles(&thiefProfiles);
    const char* guess = "N/A";

    if (museum.casefile.solved) {
        for (int i = 0; i < thiefCount; i++) {
            if (((EvidenceByte)thiefProfiles[i] & collected) == (EvidenceByte)thiefProfiles[i]) {
                guess = thief_to_string(thiefProfiles[i]);
                break;
            }
        }
    }

    printf("- Thief Guess: %s\n", guess);
    printf("- Actual Thief Type: %s\n", thief_to_string(museum.thief.type));

    bool thiefWins = (guardsWon == 0);
    printf("\nOverall Result: %s\n", thiefWins ? "Thief Wins!" : "Guards Win!");

    // cleanup museum
    museum_cleanup(&museum);
    return 0;
}

	
	/*
    1. Initialize a House structure.
    2. Populate the House with rooms using the provided helper function.
    3. Initialize all of the ghost data and hunters.
    4. Create threads for the ghost and each hunter.
    5. Wait for all threads to complete.
    6. Print final results to the console:
         - Type of ghost encountered.
         - The reason that each hunter exited
         - The evidence collected by each hunter and which ghost is represented by that evidence.
    7. Clean up all dynamically allocated resources and call sem_destroy() on all semaphores.
    */
