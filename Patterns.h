//
// Created by mat on 6/26/26.
//

#ifndef MAJAK_PATTERNS_H
#define MAJAK_PATTERNS_H
#include <stdint.h>
#include <stdbool.h>
#include "GPIO.h"

#define MAX_PATTERN_STEPS               32
#define MAX_PATTERN_LENGTH              MAX_PATTERN_STEPS
#define MAX_BRIGHTNESS                 100

///@brief Single sequence step of an emergency lighting pattern
typedef struct {
    ///@brief Duration of the step in milliseconds
    uint16_t duration_ms;
    ///@brief Bitmask of active channels during this step (Bit N enables Channel N)
    uint32_t output_mask;
} PatternStep_t;

///@brief Sequence definition for an emergency lighting pattern
typedef struct {
    ///@brief Arbitrary pattern identifier (0 to 255)
    uint8_t pattern_id;
    ///@brief Total number of sequence steps in this pattern
    uint8_t step_count;
    ///@brief Loop mode flag (true = continuous loop, false = one-shot sequence)
    bool repeat;
    ///@brief Steps array
    PatternStep_t steps[MAX_PATTERN_STEPS];
} Pattern_t;

extern Pattern_t activePattern;
extern uint8_t activePatternId;

#endif // MAJAK_PATTERNS_H