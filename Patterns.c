//
// Created by mat on 6/26/26.
//

#include "Patterns.h"
#include "stm32f1xx_hal.h"

Pattern_t activePattern = {
    {
        // red group inverted (NPN — active low)
        {
            .steps = {
                {160, true}, {20, false}, {50, true}, {290, false},
            },
            .inverted = true,
            .digitalOutputs = {
                {GPIOB, GPIO_PIN_10},
                {GPIOB, GPIO_PIN_11},
            },
        },
        // red group normal (relay — active high)
        {
            .steps = {
                {160, true}, {20, false}, {50, true}, {290, false},
            },
            .inverted = false,
            .digitalOutputs = {
                {GPIOA, GPIO_PIN_3},
            },
        },
        // blue group inverted (NPN — active low)
        {
            .steps = {
                {260, false}, {160, true}, {20, false}, {50, true}, {30, false},
            },
            .inverted = true,
            .digitalOutputs = {
                {GPIOB, GPIO_PIN_0},
                {GPIOB, GPIO_PIN_1},
            },
        },
        // blue group normal (relay — active high)
        {
            .steps = {
                {260, false}, {160, true}, {20, false}, {50, true}, {30, false},
            },
            .inverted = false,
            .digitalOutputs = {
                {GPIOA, GPIO_PIN_2},
            },
        },
    }
};

Pattern_t activePatternNight = {
    {
        // red group inverted (NPN)
        {
            .steps = {
                {160, true}, {20, false}, {50, true}, {290, false},
            },
            .inverted = true,
            .digitalOutputs = {
                {GPIOB, GPIO_PIN_10},
                {GPIOB, GPIO_PIN_11},
            },
        },
        // red group normal (relay)
        {
            .steps = {
                {160, true}, {20, false}, {50, true}, {290, false},
            },
            .inverted = false,
            .digitalOutputs = {
                {GPIOA, GPIO_PIN_3},
            },
        },
        // blue group inverted (NPN)
        {
            .steps = {
                {260, false}, {160, true}, {20, false}, {50, true}, {30, false},
            },
            .inverted = true,
            .digitalOutputs = {
                {GPIOB, GPIO_PIN_0},
                {GPIOB, GPIO_PIN_1},
            },
        },
        // blue group normal (relay)
        {
            .steps = {
                {260, false}, {160, true}, {20, false}, {50, true}, {30, false},
            },
            .inverted = false,
            .digitalOutputs = {
                {GPIOA, GPIO_PIN_2},
            },
        },
    }
};