//
// Created by mat on 6/26/26.
//

#include "Patterns.h"

const FlashStep activePattern[MAX_PATTERN_LENGTH] = {
    // red long-short
    {MAX_BRIGHTNESS, 0, 0, 160},
    {0,              0, 0,  20},
    {MAX_BRIGHTNESS, 0, 0, 50},
    {0,              0, 0,  30},
    // blue long-short
    {0, MAX_BRIGHTNESS, 0, 160},
    {0,              0, 0,  20},
    {0, MAX_BRIGHTNESS, 0, 50},
    {0,              0, 0,  30},
};
const uint16_t activePatternLength = sizeof(activePattern) / sizeof(activePattern[0]);