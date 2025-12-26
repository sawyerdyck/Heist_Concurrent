#ifndef DEFS_H
#define DEFS_H

#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_ROOM_NAME 64
#define MAX_GUARD_NAME 64
#define MAX_ROOMS 24
#define MAX_ROOM_OCCUPANCY 8
#define MAX_CONNECTIONS 8
#define ENTITY_BOREDOM_MAX 15
#define GUARD_STRESS_MAX 15
#define DEFAULT_THIEF_ID 68057

typedef unsigned char EvidenceByte; // Just giving a helpful name to unsigned char for evidence bitmasks

enum LogReason {
    LR_CLUES = 0,        // Collected enough unique clue types
    LR_BORED = 1,        // Too quiet / nothing happening
    LR_OVERWHELMED = 2   // Stress limit reached
};


enum TamperType {
    TP_CAMERA_BLACKOUT = 1 << 0,
    TP_FORCED_LOCK     = 1 << 1,
    TP_GLASS_VIBRATION = 1 << 2,
    TP_RFID_SPOOF      = 1 << 3,
    TP_MOTION_TRIGGER  = 1 << 4,
    TP_LASER_TRIP      = 1 << 5,
    TP_TOOL_MARKS      = 1 << 6,
};

enum ThiefProfile {
    TH_INSIDER        = TP_RFID_SPOOF      | TP_CAMERA_BLACKOUT | TP_FORCED_LOCK,
    TH_SMASH_AND_GRAB = TP_GLASS_VIBRATION | TP_MOTION_TRIGGER  | TP_TOOL_MARKS,
    TH_TECH_SPECIALIST= TP_CAMERA_BLACKOUT | TP_LASER_TRIP      | TP_RFID_SPOOF,
    TH_LOCKPICKER     = TP_FORCED_LOCK     | TP_TOOL_MARKS      | TP_LASER_TRIP,
    TH_ACROBAT        = TP_LASER_TRIP      | TP_MOTION_TRIGGER  | TP_CAMERA_BLACKOUT,
    TH_VANDAL         = TP_GLASS_VIBRATION | TP_TOOL_MARKS      | TP_CAMERA_BLACKOUT,
    TH_CON_ARTIST     = TP_RFID_SPOOF      | TP_MOTION_TRIGGER  | TP_GLASS_VIBRATION,
    TH_PRO            = TP_FORCED_LOCK     | TP_RFID_SPOOF      | TP_TOOL_MARKS,
    TH_OPPORTUNIST    = TP_MOTION_TRIGGER  | TP_FORCED_LOCK     | TP_GLASS_VIBRATION,
    TH_NIGHT_CRAWLER  = TP_CAMERA_BLACKOUT | TP_MOTION_TRIGGER | TP_RFID_SPOOF,
    TH_CUTTER         = TP_TOOL_MARKS      | TP_GLASS_VIBRATION | TP_LASER_TRIP,
    TH_GHOST_ENTRY    = TP_CAMERA_BLACKOUT | TP_LASER_TRIP | TP_TOOL_MARKS
};

struct CaseFile {
    EvidenceByte collected; // Union of all of the evidence bits collected between all guards
    bool         solved;    // True when >=3 unique bits set
    sem_t        mutex;     // Used for synchronizing both fields when multithreading
};


struct Room {
	char name[MAX_ROOM_NAME];
	struct Room* connectedRooms[MAX_CONNECTIONS];
	int connections;
	struct Thief* thief;
	struct Guard* guards[MAX_ROOM_OCCUPANCY];
	int guardCount;
	bool isExit;
	EvidenceByte evidence;
	sem_t mutex;
};

 
struct Thief {
	int id;
	enum ThiefProfile type;
	struct Room* currentRoom;
	int boredom;
	bool active;
	sem_t mutex;
};

struct RoomNode{
	struct Room* room;
	struct RoomNode* next;
};

struct RoomStack {
	struct RoomNode* head;
};

struct Guard{
        char name[MAX_GUARD_NAME];
        int id;
        struct Room* currentRoom;
        struct CaseFile* casefile;
        enum TamperType device;
        struct RoomStack breadcrumb;
        int stress;
        int boredom;
        enum LogReason whyExit;
        bool active;
	bool inControlRoom;
	bool returningToControl;
	bool starting;
	sem_t mutex;
};


struct Museum {
    struct Room* starting_room;
    struct Room rooms[MAX_ROOMS];
    int room_count;
    struct Guard** guards;
    int guardCount;
    int guardMax;
    struct CaseFile casefile;
    struct Thief thief;
};


//house functions
void add_guard(struct Room* room, struct Guard* guard);
void remove_guard(struct Room* room, struct Guard* guard);
void museum_init(struct Museum* museum);
void museum_cleanup(struct Museum* museum);
bool museum_add_guard(struct Museum* museum, const char* name, int id);
//ghost fucnitons
void thief_init(struct Thief* thief, struct Museum* museum);
void thief_move(struct Thief* thief);
void thief_haunt(struct Thief* thief);
void thief_update(struct Thief* thief);
//roomstack functions
void roomstack_init(struct RoomStack* stack);
void push(struct RoomStack* stack, struct Room* room);
struct Room* pop(struct RoomStack* stack);
void empty_roomstack(struct RoomStack* stack);
//room functions
void room_init(struct Room* room, const char* name, bool isExit);
void room_connect(struct Room* first, struct Room* second);
void lock_rooms(struct Room* to, struct Room* from);
void unlock_rooms(struct Room* to, struct Room* from);
void in_control_room(struct Guard* guard);
//hunter functions
void guard_init(struct Guard* guard, struct Museum* museum, const char* name, int id);
void guard_move(struct Guard* guard);
void search_for_evidence(struct Guard* guard);
bool consider_exiting(struct Guard* guard);
void update_state(struct Guard* guard);

void change_device(struct Guard* guard);
void exit_to_control_room(struct Guard* guard);
#endif // DEFS_H
