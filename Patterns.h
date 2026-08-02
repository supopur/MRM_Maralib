//
// Created by mat on 6/26/26.

#ifndef MAJAK_PATTERNS_H
#define MAJAK_PATTERNS_H
#include <stdint.h>
#include <stdbool.h>
#include "GPIO.h"


#define MAX_PATTERN_LENGTH 32
#define MAX_PATTERN_GROUPS 32
#define MAX_BRIGHTNESS 100
#define MAX_PATTERN_FLASH_GROUP_OUTPUTS 16

///@brief A single step of a FlashGroup
///@param dwell The duration of this step in ms
///@param active Whether we turn the assigned output (in FlashGroup) on or off, works for GPIO Out and also PWM (sets the highest duty cycle)
///@param intensity Is superior to active, only used for PWM output, defines to duty cycle (0-65535)
typedef struct {
    ///@brief Lenght/delay
    uint16_t dwell;
    ///@brief Only relevant if not using intensity
    bool active;
    ///@brief PWM Duty
    uint16_t intensity;
} FlashStep_t;

///@brief The pattern for a single/multiple output pins or PWM Channels
///@param steps Steps read one after another, example sequence: 100ms on, 50ms off, 120ms intensity 10000
typedef struct {
    FlashStep_t steps[MAX_PATTERN_LENGTH];

    //GPIO Output
    digitalOutput_t digitalOutputs[MAX_PATTERN_FLASH_GROUP_OUTPUTS];

    //PWM/Timer Output
    pwmOutput_t pwmOutputs[MAX_PATTERN_FLASH_GROUP_OUTPUTS];
} FlashGroup_t;

///@brief A collection of FlashGroups played at once
typedef struct {
    FlashGroup_t groups[MAX_PATTERN_GROUPS];
} Pattern_t;

///@brief Currently loaded pattern
extern Pattern_t activePattern;

uint8_t ActivePatternLenght();

#endif //MAJAK_PATTERNS_H