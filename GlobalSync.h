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

    ///@brief Cumulative step duration breakpoint table
    uint32_t prefix[MAX_PATTERN_STEPS + 1];
    uint32_t total_cycle;

    uint32_t sync_timestamp;   ///< master's tick value at the moment of last sync
    uint32_t local_ref_tick;   ///< THIS node's own HAL_GetTick() at that same moment
    bool     is_synced;
} GlobalSyncHandle;

///@brief Initialize synchronization handle.
void GlobalSync_Init(GlobalSyncHandle *handle);

///@brief Load a pattern and compute timing breakpoint table.
///@param handle Pointer to sync handle.
///@param pattern Pointer to pattern to play.
///@retval true on success, false if invalid.
bool GlobalSync_SetPattern(GlobalSyncHandle *handle, const Pattern_t *pattern);

///@brief Record a sync point.
///       master_timestamp: the tick value received from the CAN sync frame.
///       local_timestamp:  THIS node's own HAL_GetTick(), sampled as close as
///                          possible to the moment the frame was actually received
///                          (i.e. call this from the RX ISR, not the main loop).
///       These two values do NOT need to be numerically close to each other —
///       they're never subtracted from one another. Only local_timestamp is ever
///       compared against this node's own future HAL_GetTick() calls.
void GlobalSync_SetSyncPoint(GlobalSyncHandle *handle, uint32_t master_timestamp, uint32_t local_timestamp);

///@brief Get current active step index at timestamp.
uint8_t GlobalSync_GetStepIndex(const GlobalSyncHandle *handle, uint32_t timestamp);

///@brief Get active output channel bitmask at timestamp.
uint32_t GlobalSync_GetActiveMask(const GlobalSyncHandle *handle, uint32_t timestamp);

///@brief Get time remaining in current step in ms.
uint16_t GlobalSync_GetTimeRemainingInStep(const GlobalSyncHandle *handle, uint32_t timestamp);

///@brief Get current pattern phase in ms.
uint32_t GlobalSync_GetPhase(const GlobalSyncHandle *handle, uint32_t timestamp);

///@brief Get pointer to the currently active step.
const PatternStep_t *GlobalSync_GetCurrentStep(const GlobalSyncHandle *handle, uint32_t timestamp);

#endif // MAJAK_GLOBALSYNC_H