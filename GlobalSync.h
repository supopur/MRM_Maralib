//
// Created by mat on 6/25/26.
//

#ifndef MAJAK_GLOBALSYNC_H
#define MAJAK_GLOBALSYNC_H

#include <stdint.h>
#include <stdbool.h>
#include "Patterns.h"

///@brief Internal state for GPS/CAN-disciplined pattern synchronisation.
///       One handle per node; all nodes sharing the same sync timestamp
///       will produce identical step indices at any given moment.
typedef struct {
    const Pattern_t *pattern;

    /// prefix-sum of each group's dwell values (index 0 = 0, index N = total cycle)
    /// sized for the worst-case group length
    uint16_t prefix[MAX_PATTERN_LENGTH + 1];
    uint16_t total_cycle;   ///< sum of all dwell values for the active group (all groups must match)

    uint16_t sync_timestamp;
    bool     is_synced;
} GlobalSyncHandle;

///@brief Initialise the handle. Must be called before anything else.
///@param handle Pointer to the handle to initialise.
void GlobalSync_Init(GlobalSyncHandle *handle);

///@brief Load a pattern into the handle.
///       The pattern pointer must remain valid for the lifetime of the handle.
///       Can be called at any time, even after a sync point has been set.
///@param handle  Pointer to an initialised handle.
///@param pattern Pointer to the pattern to load.
///@return true on success; false if pattern is NULL, has 0 steps,
///        exceeds MAX_PATTERN_LENGTH, or total cycle overflows uint16.
bool GlobalSync_SetPattern(GlobalSyncHandle *handle, const Pattern_t *pattern);

///@brief Record the shared sync timestamp.
///       All nodes in the system must call this with the same value
///       at roughly the same moment (e.g. on receipt of a CAN SYNC frame).
///@param handle    Pointer to an initialised handle.
///@param timestamp The shared timestamp value (e.g. lower 16 bits of HAL_GetTick()).
void GlobalSync_SetSyncPoint(GlobalSyncHandle *handle, uint16_t timestamp);

///@brief Get the current step index for the given timestamp.
///@param handle    Pointer to a ready handle.
///@param timestamp Current time value (same source as SetSyncPoint).
///@return Step index (0-based), or 0xFF if not synced / no pattern loaded.
uint8_t GlobalSync_GetIndex(const GlobalSyncHandle *handle, uint16_t timestamp);

///@brief Get the number of milliseconds remaining in the current step.
///@param handle    Pointer to a ready handle.
///@param timestamp Current time value.
///@return Remaining ms, or 0 if not ready.
uint16_t GlobalSync_GetTimeRemainingInStep(const GlobalSyncHandle *handle, uint16_t timestamp);

///@brief Get the current phase — elapsed ms within the current cycle.
///@param handle    Pointer to a ready handle.
///@param timestamp Current time value.
///@return Phase in ms, or 0 if not ready.
uint16_t GlobalSync_GetPhase(const GlobalSyncHandle *handle, uint16_t timestamp);

///@brief Get the active FlashStep for a specific group at the given timestamp.
///@param handle      Pointer to a ready handle.
///@param timestamp   Current time value.
///@param group_index Index into pattern->groups[].
///@return Pointer to the current FlashStep_t, or NULL if not ready / out of range.
const FlashStep_t *GlobalSync_GetGroupStep(const GlobalSyncHandle *handle,
                                           uint16_t timestamp,
                                           uint8_t  group_index);

#endif // MAJAK_GLOBALSYNC_H