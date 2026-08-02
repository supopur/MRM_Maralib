//
// Created by mat on 6/25/26.
//

#ifndef MAJAK_GLOBALSYNC_H
#define MAJAK_GLOBALSYNC_H
#include <stdint.h>
#include <stdbool.h>

#include "Patterns.h"

typedef struct {
    const FlashStep *steps;
    uint8_t step_count;

    uint16_t prefix[MAX_PATTERN_LENGTH + 1];
    uint16_t total_cycle;

    uint16_t sync_timestamp;
    bool is_synced;
} GlobalSyncHandle;

/**
 * Initialize the handle. Must be called before anything else.
 * Does not set a pattern or sync point yet.
 */
void GlobalSync_Init(GlobalSyncHandle *handle);

/**
 * Load a new pattern. Can be called at any time, even after sync.
 * Steps array must remain valid for the lifetime of this pattern.
 * Returns false if step_count == 0 or exceeds MAX_PATTERN_LENGTH,
 * or if total cycle length overflows uint16.
 */
bool GlobalSync_SetPattern(GlobalSyncHandle *handle,
                           const FlashStep *steps,
                           uint8_t step_count);

/**
 * Record the shared sync timestamp. All nodes should call this
 * with the same timestamp value at roughly the same moment.
 */
void GlobalSync_SetSyncPoint(GlobalSyncHandle *handle, uint16_t timestamp);

/**
 * Get the current step index from a timestamp.
 * Returns 0xFF if no pattern is loaded or handle is not synced.
 */
uint8_t GlobalSync_GetIndex(const GlobalSyncHandle *handle, uint16_t timestamp);

/**
 * Get how many milliseconds remain in the current step.
 * Returns 0 if not synced or no pattern loaded.
 */
uint16_t GlobalSync_GetTimeRemainingInStep(const GlobalSyncHandle *handle, uint16_t timestamp);

/**
 * Get the current phase (elapsed ms within the current cycle).
 * Returns 0 if not synced or no pattern loaded.
 */
uint16_t GlobalSync_GetPhase(const GlobalSyncHandle *handle, uint16_t timestamp);


#endif //MAJAK_GLOBALSYNC_H
