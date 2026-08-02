//
// Created by mat on 6/26/26.
//

#include "Patterns.h"
#include "stm32f1xx_hal.h"

Pattern_t activePattern = {
    {
        // red group
        {
            {
                {160, true}, {20, false}, {50, true}, {290, false},
            },
            {
                {GPIOB, 10},
                {GPIOB, 11},
                {GPIOA, 3}
            }
        },
        // blue group
        {
            {
                {260, false}, {160, true}, {20, false}, {50, true}, {30, false}
            },
            {
                {GPIOB, 0},
                {GPIOB, 1},
                {GPIOA, 2}
            }
        }
    }
};

uint8_t ActivePatternLenght() {
    return sizeof(activePattern) / sizeof(activePattern[0]);
}