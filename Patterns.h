//
// Created by mat on 6/26/26.

#ifndef MAJAK_PATTERNS_H
#define MAJAK_PATTERNS_H
#include <stdint.h>

#define MAX_PATTERN_LENGTH 32
#define MAX_BRIGHTNESS 100

/**
 * A single step of a pattern for a single LED reflector/group consisting of 3 colors/led chains
 */
typedef struct {
    uint8_t red;
    uint8_t blue;
    uint8_t aux;
    uint16_t ms;
} FlashStep;

extern const FlashStep activePattern[];
extern const uint16_t activePatternLength;

#endif //MAJAK_PATTERNS_H