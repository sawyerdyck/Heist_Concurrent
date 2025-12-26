#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include "helpers.h"

// ---- House layout ----
void museum_populate_rooms(struct Museum* museum) {
    // Museum layout for the heist simulation (room graph kept stable).
    museum->room_count = 13;

    room_init(museum->rooms+0, "Security Office", true);
    room_init(museum->rooms+1, "Main Corridor", false);
    room_init(museum->rooms+2, "Renaissance Gallery", false);
    room_init(museum->rooms+3, "Modern Art Gallery", false);
    room_init(museum->rooms+4, "Restroom", false);
    room_init(museum->rooms+5, "Archives", false);
    room_init(museum->rooms+6, "Service Corridor", false);
    room_init(museum->rooms+7, "Storage A", false);
    room_init(museum->rooms+8, "Storage B", false);
    room_init(museum->rooms+9, "Cafe", false);
    room_init(museum->rooms+10, "Grand Hall", false);
    room_init(museum->rooms+11, "Loading Dock", false);
    room_init(museum->rooms+12, "Control Closet", false);

    room_connect(museum->rooms+0, museum->rooms+1);    // Security Office - Main Corridor
    room_connect(museum->rooms+1, museum->rooms+2);    // Main Corridor - Renaissance Gallery
    room_connect(museum->rooms+1, museum->rooms+3);    // Main Corridor - Modern Art Gallery
    room_connect(museum->rooms+1, museum->rooms+4);    // Main Corridor - Restroom
    room_connect(museum->rooms+1, museum->rooms+9);    // Main Corridor - Cafe
    room_connect(museum->rooms+1, museum->rooms+5);    // Main Corridor - Archives
    room_connect(museum->rooms+5, museum->rooms+6);    // Archives - Service Corridor
    room_connect(museum->rooms+6, museum->rooms+7);    // Service Corridor - Storage A
    room_connect(museum->rooms+6, museum->rooms+8);    // Service Corridor - Storage B
    room_connect(museum->rooms+9, museum->rooms+10);   // Cafe - Grand Hall
    room_connect(museum->rooms+9, museum->rooms+11);   // Cafe - Loading Dock
    room_connect(museum->rooms+11, museum->rooms+12);  // Loading Dock - Control Closet

    museum->starting_room = museum->rooms; // Van is at index 0
}

// ---- to_string functions ----
const char* tamper_to_string(enum TamperType t) {
    switch (t) {
        case TP_CAMERA_BLACKOUT: return "camera_blackout";
        case TP_FORCED_LOCK:     return "forced_lock";
        case TP_GLASS_VIBRATION: return "glass_vibration";
        case TP_RFID_SPOOF:      return "rfid_spoof";
        case TP_MOTION_TRIGGER:  return "motion_trigger";
        case TP_LASER_TRIP:      return "laser_trip";
        case TP_TOOL_MARKS:      return "tool_marks";
        default:                 return "unknown";
    }
}

const char* thief_to_string(enum ThiefProfile p) {
    switch (p) {
        case TH_INSIDER:         return "insider";
        case TH_SMASH_AND_GRAB:  return "smash_and_grab";
        case TH_TECH_SPECIALIST: return "tech_specialist";
        case TH_LOCKPICKER:      return "lockpicker";
        case TH_ACROBAT:         return "acrobat";
        case TH_VANDAL:          return "vandal";
        case TH_CON_ARTIST:      return "con_artist";
        case TH_PRO:             return "pro";
        case TH_OPPORTUNIST:     return "opportunist";
        case TH_NIGHT_CRAWLER:   return "night_crawler";
        case TH_CUTTER:          return "cutter";
        case TH_GHOST_ENTRY:     return "ghost_entry";
        default:                 return "unknown";
    }
}

const char* exit_reason_to_string(enum LogReason reason) {
    switch (reason) {
        case LR_CLUES:       return "clues";
        case LR_BORED:       return "bored";
        case LR_OVERWHELMED: return "overwhelmed";
        default:             return "unknown";
    }
}

// ---- enum retrieval functions ----
int get_all_tamper_types(const enum TamperType** list) {
    static const enum TamperType tamper_types[] = {
        TP_CAMERA_BLACKOUT,
        TP_FORCED_LOCK,
        TP_GLASS_VIBRATION,
        TP_RFID_SPOOF,
        TP_MOTION_TRIGGER,
        TP_LASER_TRIP,
        TP_TOOL_MARKS
    };

    if (list) *list = tamper_types;
    return (int)(sizeof(tamper_types) / sizeof(tamper_types[0]));
}

int get_all_thief_profiles(const enum ThiefProfile** list) {
    static const enum ThiefProfile profiles[] = {
        TH_INSIDER,
        TH_SMASH_AND_GRAB,
        TH_TECH_SPECIALIST,
        TH_LOCKPICKER,
        TH_ACROBAT,
        TH_VANDAL,
        TH_CON_ARTIST,
        TH_PRO,
        TH_OPPORTUNIST,
        TH_NIGHT_CRAWLER,
        TH_CUTTER,
        TH_GHOST_ENTRY
    };

    if (list) *list = profiles;
    return (int)(sizeof(profiles) / sizeof(profiles[0]));
}

// ---- Thread-safe random number generation ----
int rand_int_threadsafe(int lower_inclusive, int upper_exclusive) {
    static _Thread_local unsigned seed = 0;

    if (upper_exclusive <= lower_inclusive) {
        return lower_inclusive;
    }

    if (seed == 0) {
        seed = (unsigned)time(NULL) ^ (unsigned)(uintptr_t)pthread_self();
        if (seed == 0) {
            seed = 0xA5A5A5A5u;
        }
    }

    unsigned span = (unsigned)(upper_exclusive - lower_inclusive);
    unsigned value = (unsigned)rand_r(&seed) % span;
    return lower_inclusive + (int)value;
}

// ---- Evidence helpers ----
bool evidence_is_valid_ghost(EvidenceByte mask) {
    const enum ThiefProfile* thief_types = NULL;
    int thief_count = get_all_thief_profiles(&thief_types);

    for (int index = 0; index < thief_count; index++) {
        if (mask == (EvidenceByte)thief_types[index]) {
            return true;
        }
    }

    return false;
}

// ---- Logging (Writes CSV logs, DO NOT MODIFY the file outputs: timestamp,type,id,room,device,boredom,stress,action,extra) ----

// These enums are just for logging purposes, not needed elsewhere
enum LogEntityType {
    LOG_ENTITY_HUNTER = 0,
    LOG_ENTITY_GHOST = 1
};

struct LogRecord {
    enum LogEntityType entity_type;
    int                entity_id;
    const char*        room;
    const char*        device;
    int                boredom;
    int                stress;
    const char*        action;
    const char*        extra;
};

static const char* log_entity_type_to_string(enum LogEntityType type) {
    switch (type) {
        case LOG_ENTITY_HUNTER:
            return "hunter";
        case LOG_ENTITY_GHOST:
            return "ghost";
        default:
            return "unknown";
    }
}

static void write_log_record(const struct LogRecord* record) {
    static _Thread_local unsigned line_count = 0;

    if (line_count >= 100000) {
        fprintf(stderr, "Log capped for entity %d; stopping to prevent infinite growth.\n", record->entity_id);
        exit(1);
    }

    char filename[64];
    snprintf(filename, sizeof(filename), "log_%d.csv", record->entity_id);

    FILE* log_file = fopen(filename, "a");

    if (!log_file) {
        return;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long timestamp = (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;

    const char* entity = log_entity_type_to_string(record->entity_type);
    const char* room = record->room ? record->room : "";
    const char* device = record->device ? record->device : "";
    const char* action = record->action ? record->action : "";
    const char* extra = record->extra ? record->extra : "";

    fprintf(log_file,
            "%lld,%s,%d,%s,%s,%d,%d,%s,%s\n",
            timestamp,
            entity,
            record->entity_id,
            room,
            device,
            record->boredom,
            record->stress,
            action,
            extra);

    fclose(log_file);
    line_count++;

    // Short pause helps ensure successive logs receive distinct timestamps.
    struct timespec pause = {0, 2 * 1000 * 1000}; // 2 ms
    nanosleep(&pause, NULL);
}

void log_move(int guard_id, int boredom, int stress, const char* from_room, const char* to_room, enum TamperType device) {
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = guard_id,
        .room = from_room,
        .device = tamper_to_string(device),
        .boredom = boredom,
        .stress = stress,
        .action = "MOVE",
        .extra = to_room
    };

    write_log_record(&record);

    printf("Guard %d using %s moved from %s to %s (bored=%d stress=%d)\n",
           guard_id,
           tamper_to_string(device),
           from_room ? from_room : "",
           to_room ? to_room : "",
           boredom,
           stress);
}

void log_evidence(int guard_id, int boredom, int stress, const char* room_name, enum TamperType device) {
    const char* evidence = tamper_to_string(device);
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = guard_id,
        .room = room_name,
        .device = evidence,
        .boredom = boredom,
        .stress = stress,
        .action = "EVIDENCE",
        .extra = evidence
    };

    write_log_record(&record);

    printf("Guard %d using %s gathered evidence in %s (bored=%d stress=%d)\n",
           guard_id,
           evidence,
           room_name ? room_name : "",
           boredom,
           stress);
}

void log_swap(int guard_id, int boredom, int stress, enum TamperType from_device, enum TamperType to_device) {
    char extra[64];
    const char* from_text = tamper_to_string(from_device);
    const char* to_text = tamper_to_string(to_device);
    snprintf(extra, sizeof(extra), "%s->%s", from_text, to_text);

    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = guard_id,
        .room = NULL,
        .device = to_text,
        .boredom = boredom,
        .stress = stress,
        .action = "SWAP",
        .extra = extra
    };

    write_log_record(&record);

    printf("Guard %d swapped devices: %s -> %s (bored=%d stress=%d)\n",
           guard_id,
           from_text,
           to_text,
           boredom,
           stress);
}

void log_exit(int guard_id, int boredom, int stress, const char* room_name, enum TamperType device, enum LogReason reason) {
    const char* device_text = tamper_to_string(device);
    const char* reason_text = exit_reason_to_string(reason);

    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = guard_id,
        .room = room_name,
        .device = device_text,
        .boredom = boredom,
        .stress = stress,
        .action = "EXIT",
        .extra = reason_text
    };

    write_log_record(&record);

    printf("Guard %d using %s exited at %s (reason=%s, bored=%d stress=%d)\n",
           guard_id,
           device_text,
           room_name ? room_name : "",
           reason_text,
           boredom,
           stress);
}

void log_return_to_van(int guard_id, int boredom, int stress, const char* room_name, enum TamperType device, bool heading_home) {
    const char* device_text = tamper_to_string(device);
    const char* extra = heading_home ? "start" : "complete";
    const char* action = heading_home ? "RETURN_START" : "RETURN_COMPLETE";

    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = guard_id,
        .room = room_name,
        .device = device_text,
        .boredom = boredom,
        .stress = stress,
        .action = action,
        .extra = extra
    };

    write_log_record(&record);

    if (heading_home) {
        printf("Guard %d using %s heading to Security Office from %s (bored=%d stress=%d)\n",
               guard_id,
               device_text,
               room_name ? room_name : "",
               boredom,
               stress);
    } else {
        printf("Guard %d using %s finished return at %s (bored=%d stress=%d)\n",
               guard_id,
               device_text,
               room_name ? room_name : "",
               boredom,
               stress);
    }
}

void log_guard_init(int guard_id, const char* room_name, const char* guard_name, enum TamperType device) {
    const char* device_text = tamper_to_string(device);
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_HUNTER,
        .entity_id = guard_id,
        .room = room_name,
        .device = device_text,
        .boredom = 0,
        .stress = 0,
        .action = "INIT",
        .extra = guard_name ? guard_name : ""
    };

    write_log_record(&record);
    printf("Guard %d (%s) initialized in %s with %s\n",
           guard_id,
           guard_name ? guard_name : "unknown",
           room_name ? room_name : "",
           device_text);
}

void log_thief_init(int thief_id, const char* room_name, enum ThiefProfile type) {
    const char* type_text = thief_to_string(type);
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = thief_id,
        .room = room_name,
        .device = NULL,
        .boredom = 0,
        .stress = 0,
        .action = "INIT",
        .extra = type_text
    };

    write_log_record(&record);
    printf("Thief %d (%s) initialized in %s\n",
           thief_id,
           type_text,
           room_name ? room_name : "");
}

void log_thief_move(int thief_id, int boredom, const char* from_room, const char* to_room) {
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = thief_id,
        .room = from_room,
        .device = NULL,
        .boredom = boredom,
        .stress = 0,
        .action = "MOVE",
        .extra = to_room
    };

    write_log_record(&record);

    printf("Thief %d [bored=%d] MOVE %s -> %s\n",
           thief_id,
           boredom,
           from_room ? from_room : "",
           to_room ? to_room : "");
}

void log_thief_evidence(int thief_id, int boredom, const char* room_name, enum TamperType evidence) {
    const char* evidence_text = tamper_to_string(evidence);

    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = thief_id,
        .room = room_name,
        .device = NULL,
        .boredom = boredom,
        .stress = 0,
        .action = "EVIDENCE",
        .extra = evidence_text
    };

    write_log_record(&record);

    printf("Thief %d [bored=%d] EVIDENCE %s in %s\n",
           thief_id,
           boredom,
           evidence_text,
           room_name ? room_name : "");
}

void log_thief_exit(int thief_id, int boredom, const char* room_name) {
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = thief_id,
        .room = room_name,
        .device = NULL,
        .boredom = boredom,
        .stress = 0,
        .action = "EXIT",
        .extra = ""
    };

    write_log_record(&record);

    printf("Thief %d [bored=%d] EXIT %s\n",
           thief_id,
           boredom,
           room_name ? room_name : "");
}

void log_thief_idle(int thief_id, int boredom, const char* room_name) {
    struct LogRecord record = {
        .entity_type = LOG_ENTITY_GHOST,
        .entity_id = thief_id,
        .room = room_name,
        .device = NULL,
        .boredom = boredom,
        .stress = 0,
        .action = "IDLE",
        .extra = ""
    };

    write_log_record(&record);

    printf("Thief %d [bored=%d] IDLE in %s\n",
           thief_id,
           boredom,
           room_name ? room_name : "");
}

