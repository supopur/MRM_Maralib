#include "GlobalSync.h"
#include <string.h>

static bool is_ready(const GlobalSyncHandle *handle) {
    return handle->is_synced
        && handle->pattern != NULL
        && handle->total_cycle > 0;
}

// elapsed ms within the current cycle, computed WITHOUT ever mixing
// two different clocks in one subtraction:
//   - (timestamp - local_ref_tick) is this node's own clock vs itself: always valid.
//   - sync_timestamp (master's phase reference) is only ever ADDED, never subtracted
//     from a local tick.
static uint32_t compute_phase(const GlobalSyncHandle *handle, uint32_t timestamp)
{
    uint32_t elapsed_local = timestamp - handle->local_ref_tick; // same-clock diff, wraps correctly
    uint32_t phase         = elapsed_local + handle->sync_timestamp;
    return phase % handle->total_cycle;
}

static uint8_t phase_to_index(const GlobalSyncHandle *handle, uint32_t phase) {
    for (uint16_t i = 0; i < MAX_PATTERN_LENGTH; i++) {
        if (handle->prefix[i + 1] == 0) break;
        if (phase < handle->prefix[i + 1])
            return (uint8_t)i;
    }
    return 0;
}

void GlobalSync_Init(GlobalSyncHandle *handle) {
    memset(handle, 0, sizeof(GlobalSyncHandle));
}

bool GlobalSync_SetPattern(GlobalSyncHandle *handle, const Pattern_t *pattern) {
    if (pattern == NULL)
        return false;

    const FlashGroup_t *ref = &pattern->groups[0];
    memset(handle->prefix, 0, sizeof(handle->prefix));

    uint32_t accum = 0;
    handle->prefix[0] = 0;
    uint8_t step_count = 0;

    for (uint8_t i = 0; i < MAX_PATTERN_LENGTH; i++) {
        if (ref->steps[i].dwell == 0) break;
        accum += ref->steps[i].dwell;
        if (accum > UINT16_MAX)
            return false;
        handle->prefix[i + 1] = (uint16_t)accum;
        step_count = i + 1;
    }

    if (step_count == 0)
        return false;

    handle->pattern     = pattern;
    handle->total_cycle = (uint16_t)accum;
    return true;
}

void GlobalSync_SetSyncPoint(GlobalSyncHandle *handle, uint32_t master_timestamp, uint32_t local_timestamp) {
    handle->sync_timestamp = master_timestamp;
    handle->local_ref_tick = local_timestamp;
    handle->is_synced      = true;
}

uint8_t GlobalSync_GetIndex(const GlobalSyncHandle *handle, uint32_t timestamp) {
    if (!is_ready(handle))
        return 0xFF;
    return phase_to_index(handle, compute_phase(handle, timestamp));
}

uint16_t GlobalSync_GetTimeRemainingInStep(const GlobalSyncHandle *handle, uint32_t timestamp) {
    if (!is_ready(handle))
        return 0;
    uint32_t phase = compute_phase(handle, timestamp);
    uint8_t  idx   = phase_to_index(handle, phase);
    return handle->prefix[idx + 1] - (uint16_t)phase;
}

uint16_t GlobalSync_GetPhase(const GlobalSyncHandle *handle, uint32_t timestamp) {
    if (!is_ready(handle))
        return 0;
    return (uint16_t)compute_phase(handle, timestamp);
}

const FlashStep_t *GlobalSync_GetGroupStep(const GlobalSyncHandle *handle,
                                           uint32_t timestamp,
                                           uint8_t  group_index) {
    if (!is_ready(handle))
        return NULL;
    if (group_index >= MAX_PATTERN_GROUPS)
        return NULL;

    uint8_t idx = GlobalSync_GetIndex(handle, timestamp);
    if (idx == 0xFF)
        return NULL;

    return &handle->pattern->groups[group_index].steps[idx];
}