//
// Created by mat on 6/26/26.
//

#ifndef MAJAK_PATTERNS_H
#define MAJAK_PATTERNS_H
#include <stdint.h>
#include <stdbool.h>
#include "GPIO.h"

#define MAX_PATTERN_LENGTH              16
#define MAX_PATTERN_GROUPS               4
#define MAX_BRIGHTNESS                 100
#define MAX_PATTERN_FLASH_GROUP_OUTPUTS 16

///@brief A single step of a FlashGroup
typedef struct {
    ///@brief Duration in ms
    uint16_t dwell;
    ///@brief Output state — ignored when intensity > 0
    bool active;
    ///@brief PWM duty cycle (0–65535); when > 0 overrides active
    uint16_t intensity;
} FlashStep_t;

///@brief Pattern for one or more output pins / PWM channels
///@note  When inverted = true the output is active-low:
///         active=true  → GPIO LOW  (NPN driven, transistor on)
///         active=false → GPIO HIGH (NPN off, output pulled high)
///       The idle/off state (lights disabled) is also inverted automatically.
typedef struct {
    FlashStep_t steps[MAX_PATTERN_LENGTH];

    ///@brief Set true for active-low (NPN) outputs
    bool inverted;

    // GPIO outputs
    digitalOutput_t digitalOutputs[MAX_PATTERN_FLASH_GROUP_OUTPUTS];

    // PWM/Timer outputs
    pwmOutput_t pwmOutputs[MAX_PATTERN_FLASH_GROUP_OUTPUTS];
} FlashGroup_t;

///@brief A collection of FlashGroups played simultaneously
typedef struct {
    FlashGroup_t groups[MAX_PATTERN_GROUPS];
} Pattern_t;

extern Pattern_t activePattern;
extern Pattern_t activePatternNight;

#endif // MAJAK_PATTERNS_H