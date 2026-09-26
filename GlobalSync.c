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
    for (uint8_t i = 0; i < handle->pattern->step_count; i++) {
        if (phase < handle->prefix[i + 1]) {
            return i;
        }
    }
    return 0;
}

void GlobalSync_Init(GlobalSyncHandle *handle) {
    memset(handle, 0, sizeof(GlobalSyncHandle));
}

bool GlobalSync_SetPattern(GlobalSyncHandle *handle, const Pattern_t *pattern) {
    if (pattern == NULL || pattern->step_count == 0 || pattern->step_count > MAX_PATTERN_STEPS) {
        return false;
    }

    memset(handle->prefix, 0, sizeof(handle->prefix));

    uint32_t accum = 0;
    handle->prefix[0] = 0;

    for (uint8_t i = 0; i < pattern->step_count; i++) {
        accum += pattern->steps[i].duration_ms;
        handle->prefix[i + 1] = accum;
    }

    if (accum == 0) {
        return false;
    }

    handle->pattern = pattern;
    handle->total_cycle = accum;
    return true;
}

void GlobalSync_SetSyncPoint(GlobalSyncHandle *handle, uint32_t master_timestamp, uint32_t local_timestamp) {
    handle->sync_timestamp = master_timestamp;
    handle->local_ref_tick = local_timestamp;
    handle->is_synced      = true;
}

uint8_t GlobalSync_GetStepIndex(const GlobalSyncHandle *handle, uint32_t timestamp) {
    if (!is_ready(handle)) {
        return 0xFF;
    }

    // handle one-shot non-repeating pattern termination
    if (!handle->pattern->repeat) {
        uint32_t elapsed_local = timestamp - handle->local_ref_tick;
        uint32_t total_elapsed = elapsed_local + handle->sync_timestamp;
        if (total_elapsed >= handle->total_cycle) {
            return 0xFF;
        }
    }

    uint32_t phase = compute_phase(handle, timestamp);
    return phase_to_index(handle, phase);
}

uint32_t GlobalSync_GetActiveMask(const GlobalSyncHandle *handle, uint32_t timestamp) {
    uint8_t idx = GlobalSync_GetStepIndex(handle, timestamp);
    if (idx == 0xFF) {
        return 0;
    }
    return handle->pattern->steps[idx].output_mask;
}

uint16_t GlobalSync_GetTimeRemainingInStep(const GlobalSyncHandle *handle, uint32_t timestamp) {
    if (!is_ready(handle)) {
        return 0;
    }

    uint8_t idx = GlobalSync_GetStepIndex(handle, timestamp);
    if (idx == 0xFF) {
        return 0;
    }

    uint32_t phase = compute_phase(handle, timestamp);
    if (handle->prefix[idx + 1] > phase) {
        return (uint16_t)(handle->prefix[idx + 1] - phase);
    }
    return 0;
}

uint32_t GlobalSync_GetPhase(const GlobalSyncHandle *handle, uint32_t timestamp) {
    if (!is_ready(handle)) {
        return 0;
    }
    return compute_phase(handle, timestamp);
}

const PatternStep_t *GlobalSync_GetCurrentStep(const GlobalSyncHandle *handle, uint32_t timestamp) {
    uint8_t idx = GlobalSync_GetStepIndex(handle, timestamp);
    if (idx == 0xFF) {
        return NULL;
    }
    return &handle->pattern->steps[idx];
}