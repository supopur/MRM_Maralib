//
// Created by mat on 6/25/26.
//

#include "GlobalSync.h"

#include <stdbool.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                    */
/* ------------------------------------------------------------------ */

static bool is_ready(const GlobalSyncHandle *handle) {
    return handle->is_synced && handle->step_count > 0;
}

static uint16_t compute_phase(const GlobalSyncHandle *handle, uint16_t timestamp) {
    uint16_t elapsed = timestamp - handle->sync_timestamp; /* wraps correctly */
    return elapsed % handle->total_cycle;
}

static uint8_t phase_to_index(const GlobalSyncHandle *handle, uint16_t phase) {
    for (uint8_t i = 0; i < handle->step_count; i++) {
        if (phase < handle->prefix[i + 1])
            return i;
    }
    return handle->step_count - 1; /* shouldn't happen */
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void GlobalSync_Init(GlobalSyncHandle *handle) {
    memset(handle, 0, sizeof(GlobalSyncHandle));
}

bool GlobalSync_SetPattern(GlobalSyncHandle *handle,
                           const FlashStep *steps,
                           uint8_t step_count) {
    if (step_count == 0 || step_count > MAX_PATTERN_LENGTH)
        return false;

    /* Build prefix sums, check for uint16 overflow */
    uint32_t accum = 0;
    handle->prefix[0] = 0;
    for (uint8_t i = 0; i < step_count; i++) {
        accum += steps[i].ms;
        if (accum > UINT16_MAX)
            return false;
        handle->prefix[i + 1] = (uint16_t) accum;
    }

    handle->steps = steps;
    handle->step_count = step_count;
    handle->total_cycle = (uint16_t) accum;

    return true;
}

void GlobalSync_SetSyncPoint(GlobalSyncHandle *handle, uint16_t timestamp) {
    handle->sync_timestamp = timestamp;
    handle->is_synced = true;
}

uint8_t GlobalSync_GetIndex(const GlobalSyncHandle *handle, uint16_t timestamp) {
    if (!is_ready(handle))
        return 0xFF;

    uint16_t phase = compute_phase(handle, timestamp);
    return phase_to_index(handle, phase);
}

uint16_t GlobalSync_GetTimeRemainingInStep(const GlobalSyncHandle *handle, uint16_t timestamp) {
    if (!is_ready(handle))
        return 0;

    uint16_t phase = compute_phase(handle, timestamp);
    uint8_t idx = phase_to_index(handle, phase);

    return handle->prefix[idx + 1] - phase;
}

uint16_t GlobalSync_GetPhase(const GlobalSyncHandle *handle, uint16_t timestamp) {
    if (!is_ready(handle))
        return 0;

    return compute_phase(handle, timestamp);
}
