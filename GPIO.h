//
// Created by mat on 8/2/26.
//

#ifndef CODE3MASTER_GPIO_H
#define CODE3MASTER_GPIO_H
#include <stdint.h>
#include "stm32f1xx_hal.h"

typedef struct {
    GPIO_TypeDef *outputPort;
    uint16_t outputPin;
} digitalOutput_t;

typedef struct {
    TIM_HandleTypeDef *pwmTimer;
    uint32_t pwmChannel;
} pwmOutput_t;

#endif //CODE3MASTER_GPIO_H