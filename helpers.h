#ifndef HELPERS_H
#define HELPERS_H

#include "defs.h"

/**
 * @brief Return the lowercase token for a device.
 * @param[in] evidence  Evidence type value.
 * @return Static string such as "emf"; "unknown" when out of range.
 */
const char* tamper_to_string(enum TamperType evidence);

/**
 * @brief Return the lowercase token for a ghost type.
 * @param[in] ghost Thief type value.
 * @return Static string such as "goryo"; "unknown" when out of range.
 */
const char* thief_to_string(enum ThiefProfile ghost);

/**
 * @brief Translate a log reason to text.
 * @param[in] reason Exit reason enum.
 * @return Static string like "bored".
 */
const char* exit_reason_to_string(enum LogReason reason);

/**
 * @brief Expose every evidence device.
 * @param[out] list Optional pointer updated to an array of seven entries.
 * @return Number of items in the returned list.
 */
int get_all_tamper_types(const enum TamperType** list);

/**
 * @brief Expose every ghost archetype.
 * @param[out] list Optional pointer updated to an array of ghost types.
 * @return Number of ghost entries in the array.
 */
int get_all_thief_profiles(const enum ThiefProfile** list);

/**
 * @brief Thread-safe random integer helper.
 * @param[in] lower_inclusive Minimum value (inclusive).
 * @param[in] upper_exclusive Maximum value (exclusive).
 * @return Random number in [lower_inclusive, upper_exclusive).
 */
int rand_int_threadsafe(int lower_inclusive, int upper_exclusive);

/**
 * @brief Verify whether an evidence mask matches a supported ghost type.
 * @param[in] mask Combined evidence mask.
 * @return true when the mask maps exactly to a ghost definition.
 */
bool evidence_is_valid_ghost(EvidenceByte mask);

/**
 * @brief Check whether at least three unique evidence bits are set.
 * @param[in] mask Evidence bitmask to inspect.
 * @return true when three or more distinct bits are set.
 */
bool evidence_has_three_unique(EvidenceByte mask);

/**
 * @brief Populate the museum structure with the Willow layout.
 * @param[in,out] museum Museum to populate; starting_room is set to the van.
 */
void museum_populate_rooms(struct Museum* museum);

/**
 * @brief Append a MOVE entry for a hunter.
 * @param[in] id Guard identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] stress Current stress level.
 * @param[in] from Source room name.
 * @param[in] to Destination room name.
 * @param[in] device Device the hunter is holding.
 */
void log_move(int id, int boredom, int stress, const char* from, const char* to, enum TamperType device);

/**
 * @brief Append an EVIDENCE entry for a hunter.
 * @param[in] id Guard identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] stress Current stress level.
 * @param[in] room Room where evidence was collected.
 * @param[in] device Device used to collect evidence.
 */
void log_evidence(int id, int boredom, int stress, const char* room, enum TamperType device);

/**
 * @brief Append a SWAP entry for a hunter.
 * @param[in] id Guard identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] stress Current stress level.
 * @param[in] from Device swapped from.
 * @param[in] to Device swapped to.
 */
void log_swap(int id, int boredom, int stress, enum TamperType from, enum TamperType to);

/**
 * @brief Append an EXIT entry for a hunter.
 * @param[in] id Guard identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] stress Current stress level.
 * @param[in] room Exit room name.
 * @param[in] device Device carried.
 * @param[in] reason Exit reason.
 */
void log_exit(int id, int boredom, int stress, const char* room, enum TamperType device, enum LogReason reason);

/**
 * @brief Append a MOVE entry for the ghost.
 * @param[in] id Thief identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] from Source room.
 * @param[in] to Destination room.
 */
void log_thief_move(int id, int boredom, const char* from, const char* to);

/**
 * @brief Append an EVIDENCE entry for the ghost.
 * @param[in] id Thief identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] room Room where evidence was dropped.
 * @param[in] evidence Evidence type left behind.
 */
void log_thief_evidence(int id, int boredom, const char* room, enum TamperType evidence);

/**
 * @brief Append an EXIT entry for the ghost.
 * @param[in] id Thief identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] room Room the ghost leaves from.
 */
void log_thief_exit(int id, int boredom, const char* room);

/**
 * @brief Append an IDLE entry for the ghost.
 * @param[in] id Thief identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] room Room the ghost stays in.
 */
void log_thief_idle(int id, int boredom, const char* room);

/**
 * @brief Append a RETURN entry for the hunter.
 * @param[in] id Guard identifier.
 * @param[in] boredom Current boredom level.
 * @param[in] stress Current stress level.
 * @param[in] room Room the hunter is currently in.
 * @param[in] device Device being carried.
 * @param[in] heading_home true if beginning the return path.
 */
void log_return_to_van(int id, int boredom, int stress, const char* room, enum TamperType device, bool heading_home);

/**
 * @brief Append an INIT entry for a hunter.
 * @param[in] id Guard identifier.
 * @param[in] room Starting room.
 * @param[in] name Guard name.
 * @param[in] device Initial device.
 */
void log_guard_init(int id, const char* room, const char* name, enum TamperType device);

/**
 * @brief Append an INIT entry for the ghost.
 * @param[in] id Thief identifier.
 * @param[in] room Starting room.
 * @param[in] type Thief type.
 */
void log_thief_init(int id, const char* room, enum ThiefProfile type);

#endif // HELPERS_H

