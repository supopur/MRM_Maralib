//
// Created by mat on 6/26/26.
//

#include "Patterns.h"
#include "stm32f1xx_hal.h"

Pattern_t activePattern = {
    {
        // red group inverted (npn)
        {
            {
                {160, false}, {20, true}, {50, false}, {290, true},
            },
            {
                {GPIOB, 10},
                {GPIOB, 11}
            }
        },
        // red group normal (relay)
        {
            {
                {160, true}, {20, false}, {50, true}, {290, false},
            },
            {
                {GPIOA, 3}
            }
        },
        // blue group inverted (npn)
        {
            {
                {260, true}, {160, false}, {20, true}, {50, false}, {30, true}
            },
            {
                {GPIOB, 0},
                {GPIOB, 1}
            }
        },
        // blue group normal (relay)
        {
            {
                {260, false}, {160, true}, {20, false}, {50, true}, {30, false}
            },
            {
                {GPIOA, 2}
            }
        }
    }
};

Pattern_t activePatternNight = {
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
