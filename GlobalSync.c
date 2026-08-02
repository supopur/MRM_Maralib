//
// Created by mat on 6/25/26.
//

#include "GlobalSync.h"
#include <string.h>

// Internal: true when the handle has a pattern loaded and a sync point set
static bool is_ready(const GlobalSyncHandle *handle) {
    return handle->is_synced
        && handle->pattern != NULL
        && handle->total_cycle > 0;
}

// Internal: elapsed ms within the current cycle (wraps correctly on uint16 overflow)
static uint32_t compute_phase(const GlobalSyncHandle *handle, uint32_t timestamp)
{
    uint32_t elapsed = timestamp - handle->sync_timestamp;
    return elapsed % handle->total_cycle;
}

// Internal: binary-search the prefix array to find which step owns the given phase
static uint8_t phase_to_index(const GlobalSyncHandle *handle, uint32_t phase) {
    // prefix[0] = 0, prefix[N] = total_cycle
    // find i such that prefix[i] <= phase < prefix[i+1]
    for (uint16_t i = 0; i < MAX_PATTERN_LENGTH; i++) {
        if (handle->prefix[i + 1] == 0) break; // end of loaded steps
        if (phase < handle->prefix[i + 1])
            return (uint8_t)i;
    }
    return 0; // shouldn't happen if prefix is consistent
}

void GlobalSync_Init(GlobalSyncHandle *handle) {
    memset(handle, 0, sizeof(GlobalSyncHandle));
}

bool GlobalSync_SetPattern(GlobalSyncHandle *handle, const Pattern_t *pattern) {
    if (pattern == NULL)
        return false;

    // Use group 0 as the timing reference — all groups must have identical dwell sequences
    const FlashGroup_t *ref = &pattern->groups[0];

    // Clear prefix array to remove any stale entries from a previous pattern
    memset(handle->prefix, 0, sizeof(handle->prefix));

    uint32_t accum = 0;
    handle->prefix[0] = 0;
    uint8_t step_count = 0;

    for (uint8_t i = 0; i < MAX_PATTERN_LENGTH; i++) {
        if (ref->steps[i].dwell == 0) break; // zero dwell = end of sequence
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

void GlobalSync_SetSyncPoint(GlobalSyncHandle *handle, uint32_t timestamp) {
    handle->sync_timestamp = timestamp;
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