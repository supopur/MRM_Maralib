#include "GlobalSync.h"
#include <string.h>
#include "DviLut.h"

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
    if (!is_ready(handle)) {
        return 0;
    }
    uint32_t mask = 0;
    for (uint8_t ch = 0; ch < MAX_PATTERN_CHANNELS; ch++) {
        if (GlobalSync_GetChannelBrightness(handle, ch, timestamp) > 0) {
            mask |= (1UL << ch);
        }
    }
    return mask;
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

uint16_t GlobalSync_GetElapsedInStep(const GlobalSyncHandle *handle, uint32_t timestamp) {
    if (!is_ready(handle)) {
        return 0;
    }

    uint8_t idx = GlobalSync_GetStepIndex(handle, timestamp);
    if (idx == 0xFF) {
        return 0;
    }

    uint32_t phase = compute_phase(handle, timestamp);
    if (phase >= handle->prefix[idx]) {
        return (uint16_t)(phase - handle->prefix[idx]);
    }
    return 0;
}

uint8_t GlobalSync_GetPrevStepIndex(const GlobalSyncHandle *handle, uint8_t curr_idx) {
    if (handle == NULL || handle->pattern == NULL || curr_idx == 0xFF || handle->pattern->step_count == 0) {
        return 0xFF;
    }
    if (curr_idx > 0) {
        return curr_idx - 1;
    }
    if (handle->pattern->repeat) {
        return handle->pattern->step_count - 1;
    }
    return 0xFF;
}

uint8_t GlobalSync_GetNextStepIndex(const GlobalSyncHandle *handle, uint8_t curr_idx) {
    if (handle == NULL || handle->pattern == NULL || curr_idx == 0xFF || handle->pattern->step_count == 0) {
        return 0xFF;
    }
    if (curr_idx + 1 < handle->pattern->step_count) {
        return curr_idx + 1;
    }
    if (handle->pattern->repeat) {
        return 0;
    }
    return 0xFF;
}

uint8_t GlobalSync_GetChannelBrightness(const GlobalSyncHandle *handle, uint8_t channel_id, uint32_t timestamp) {
    if (!is_ready(handle) || channel_id >= MAX_PATTERN_CHANNELS) {
        return 0;
    }

    uint8_t curr_idx = GlobalSync_GetStepIndex(handle, timestamp);
    if (curr_idx == 0xFF) {
        return 0;
    }

    const Pattern_t *pat = handle->pattern;
    const PatternStep_t *curr = &pat->steps[curr_idx];
    const ChannelKeyframe_t *curr_kf = &curr->channels[channel_id];

    uint8_t prev_idx = GlobalSync_GetPrevStepIndex(handle, curr_idx);
    uint8_t next_idx = GlobalSync_GetNextStepIndex(handle, curr_idx);
    const PatternStep_t *prev = (prev_idx != 0xFF) ? &pat->steps[prev_idx] : NULL;
    const PatternStep_t *next = (next_idx != 0xFF) ? &pat->steps[next_idx] : NULL;

    const ChannelKeyframe_t *prev_kf = (prev != NULL) ? &prev->channels[channel_id] : NULL;
    const ChannelKeyframe_t *next_kf = (next != NULL) ? &next->channels[channel_id] : NULL;

    uint8_t b_curr = curr_kf->brightness;
    uint8_t b_prev = (prev_kf != NULL) ? prev_kf->brightness : 0;
    uint8_t b_next = (next_kf != NULL) ? next_kf->brightness : 0;

    if (b_curr == 0 && b_prev == 0 && b_next == 0) {
        return 0;
    }

    uint16_t dur = curr->duration_ms;
    if (dur == 0) {
        return b_curr;
    }

    uint16_t t = GlobalSync_GetElapsedInStep(handle, timestamp);

    uint16_t fade_in = FADE_UNITS_TO_MS(curr_kf->fade_in_units);
    uint16_t fade_out = FADE_UNITS_TO_MS(curr_kf->fade_out_units);
    uint16_t eff_fade_in;
    uint16_t eff_fade_out;

    if ((uint32_t)fade_in + (uint32_t)fade_out > dur) {
        eff_fade_in = (uint16_t)(((uint32_t)fade_in * dur) / ((uint32_t)fade_in + (uint32_t)fade_out));
        eff_fade_out = (uint16_t)(dur - eff_fade_in);
    } else {
        eff_fade_in = fade_in;
        eff_fade_out = fade_out;
    }

    // If previous step already executed a fade-out to this step's brightness,
    // the channel starts at b_curr, avoiding double-fading.
    uint8_t start_b = (prev_kf != NULL && b_prev > 0 && prev_kf->fade_out_units > 0) ? b_curr : b_prev;

    // Fade-in region
    if (t < eff_fade_in && eff_fade_in > 0) {
        if (start_b == b_curr) {
            return b_curr;
        }
        uint32_t lut_idx = ((uint32_t)t * (DVI_LUT_SIZE - 1U)) / eff_fade_in;
        if (lut_idx >= DVI_LUT_SIZE) lut_idx = DVI_LUT_SIZE - 1U;
        uint16_t ease = DVI_EASE_LUT[lut_idx];
        int32_t diff = (int32_t)b_curr - (int32_t)start_b;
        int32_t b = (int32_t)start_b + (diff * (int32_t)ease + 32768) / (int32_t)DVI_LUT_MAX_VALUE;
        if (b < 0) b = 0;
        if (b > MAX_BRIGHTNESS) b = MAX_BRIGHTNESS;
        return (uint8_t)b;
    }

    // Fade-out region
    if (eff_fade_out > 0 && t >= (dur - eff_fade_out)) {
        if (b_curr == b_next) {
            return b_curr;
        }
        uint32_t fade_out_start = dur - eff_fade_out;
        uint32_t t_out = t - fade_out_start;
        uint32_t lut_idx = (t_out * (DVI_LUT_SIZE - 1U)) / eff_fade_out;
        if (lut_idx >= DVI_LUT_SIZE) lut_idx = DVI_LUT_SIZE - 1U;
        uint16_t ease = DVI_EASE_LUT[lut_idx];
        int32_t diff = (int32_t)b_next - (int32_t)b_curr;
        int32_t b = (int32_t)b_curr + (diff * (int32_t)ease + 32768) / (int32_t)DVI_LUT_MAX_VALUE;
        if (b < 0) b = 0;
        if (b > MAX_BRIGHTNESS) b = MAX_BRIGHTNESS;
        return (uint8_t)b;
    }

    // Dwell region
    return b_curr;
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
