#include "defs.h"
#include <stdlib.h>
/**
 * @brief initialize a roomstack
 *
 * initializes new empty stack
 *
 * @param[out] stack pointer to the roomstack
 */
void roomstack_init(struct RoomStack* stack){
    stack->head = NULL;
}

/**
 * @brief push a room onto the top of the stack.
 *
 * allocates new roomnode and inserts it at the front of the list
 *
 * @param[in,out] stack pointer to the roomstack 
 * @param[in] room pointer to the room being pushed.
 */
void push(struct RoomStack* stack, struct Room* room){
    struct RoomNode* node = malloc(sizeof(struct RoomNode));
    if (!node){
        return;
    }

    node->room = room;
    node->next = stack->head;
    stack->head = node;
}

/**
 * @brief pop the top room from the stack.
 *
 * removes and returns most recently pushed room
 *
 * @param[in,out] stack pointer to roomdtack
 *
 * @return pointer to popped room
 */
struct Room* pop(struct RoomStack* stack){
    if (!stack->head){
        return NULL;
    }

    struct RoomNode* node = stack->head;
    struct Room* room = node->room;
    stack->head = node->next;
    free(node);

    return room;
}

/**
 * @brief empty a roomstack
 *
 * iterates through linked list and frees each node.
 *
 * @param[in,out] stack pointer to the breadcrumb
 */
void empty_roomstack(struct RoomStack* stack){
    while (stack->head){
        struct RoomNode* node = stack->head;
        stack->head = node->next;
        free(node);
    }
}

