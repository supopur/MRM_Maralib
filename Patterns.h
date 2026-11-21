//
// Created by mat on 6/26/26.
//

#ifndef MAJAK_PATTERNS_H
#define MAJAK_PATTERNS_H
#include <stdint.h>
#include <stdbool.h>
#include "GPIO.h"
#include "ProductConfig.h"

#define MAX_PATTERN_STEPS               16
#define MAX_PATTERN_LENGTH              MAX_PATTERN_STEPS
#define MAX_BRIGHTNESS                 100

#if (ACTIVE_PRODUCT == PRODUCT_ID_CODE3_LEDX)
#define MAX_PATTERN_CHANNELS            16
#else
// Baboon, Marmoset, Baboon_Dev (PA8=TIM1_CH1, PA9=TIM1_CH2, PA10=TIM1_CH3)
#define MAX_PATTERN_CHANNELS            4
#endif

#define FADE_UNIT_MS                    4U
#define FADE_MS_TO_UNITS(ms)            ((uint8_t)(((uint32_t)(ms) + (FADE_UNIT_MS / 2U)) / FADE_UNIT_MS > 255U ? 255U : (((uint32_t)(ms) + (FADE_UNIT_MS / 2U)) / FADE_UNIT_MS)))
#define FADE_UNITS_TO_MS(units)         ((uint16_t)((uint32_t)(units) * FADE_UNIT_MS))

///@brief Keyframe properties for a single channel at a specific step coordinate (Step, Channel)
typedef struct __attribute__((packed)) {
    ///@brief Target brightness percentage (0 to MAX_BRIGHTNESS, 0 = off)
    uint8_t brightness;
    ///@brief Fade-in duration in 4ms units (0..255 -> 0..1020ms)
    uint8_t fade_in_units;
    ///@brief Fade-out duration in 4ms units (0..255 -> 0..1020ms)
    uint8_t fade_out_units;
} ChannelKeyframe_t;

///@brief Single sequence step along the time axis, containing independent keyframes for all channels
typedef struct __attribute__((packed)) {
    ///@brief Duration of the step in milliseconds
    uint16_t duration_ms;
    ///@brief Keyframe properties for each channel coordinate at this step
    ChannelKeyframe_t channels[MAX_PATTERN_CHANNELS];
} PatternStep_t;

///@brief Sequence definition for an emergency lighting pattern
typedef struct __attribute__((packed)) {
    ///@brief Arbitrary pattern identifier (0 to 255)
    uint8_t pattern_id;
    ///@brief Total number of sequence steps in this pattern
    uint8_t step_count;
    ///@brief Loop mode flag (true = continuous loop, false = one-shot sequence)
    bool repeat;
    ///@brief Reserved for alignment
    uint8_t reserved;
    ///@brief Steps array
    PatternStep_t steps[MAX_PATTERN_STEPS];
} Pattern_t;

extern Pattern_t activePattern;
extern uint8_t activePatternId;

#endif // MAJAK_PATTERNS_H
