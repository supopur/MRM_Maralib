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

    ///@brief Per-group cumulative dwell (breakpoint) tables.
    ///       Each group can have its own step timing, as long as every
    ///       non-empty group's total dwell equals total_cycle (they must
    ///       all complete one full cycle together to stay in sync).
    uint16_t prefix[MAX_PATTERN_GROUPS][MAX_PATTERN_LENGTH + 1];
    uint16_t total_cycle;

    uint32_t sync_timestamp;   ///< master's tick value at the moment of last sync
    uint32_t local_ref_tick;   ///< THIS node's own HAL_GetTick() at that same moment
    bool     is_synced;
} GlobalSyncHandle;

void GlobalSync_Init(GlobalSyncHandle *handle);

///@brief Load a pattern and build per-group breakpoint tables.
///       Every group that defines at least one step (dwell != 0) must sum
///       to the same total_cycle as the others, so all channels complete
///       one loop together. Groups with no steps defined are treated as
///       permanently off and are skipped during validation.
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

uint8_t GlobalSync_GetIndex(const GlobalSyncHandle *handle, uint32_t timestamp, uint8_t group_index);
uint16_t GlobalSync_GetTimeRemainingInStep(const GlobalSyncHandle *handle, uint32_t timestamp, uint8_t group_index);
uint16_t GlobalSync_GetPhase(const GlobalSyncHandle *handle, uint32_t timestamp);
const FlashStep_t *GlobalSync_GetGroupStep(const GlobalSyncHandle *handle,
                                           uint32_t timestamp,
                                           uint8_t  group_index);

#endif // MAJAK_GLOBALSYNC_H