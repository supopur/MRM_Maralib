//
// Created by mat on 6/26/26.
//

#ifndef MAJAK_LIGHTUTILS_H
#define MAJAK_LIGHTUTILS_H
#include <stdint.h>
#include "stm32f1xx_hal.h"

#define P_CONTROLLER_GAIN 120

uint16_t PercentageToARR(float percentage);

uint8_t ARRToPercentage(uint16_t arr);

// always use this when setting LED PWM!!! This accounts for the current temperature
uint16_t SetPWM(uint16_t dutyCycle, uint32_t Channel, TIM_HandleTypeDef* tim);

void UpdateCurrentTemp(uint16_t currentTempReading, uint16_t targetTempReading);

#endif //MAJAK_LIGHTUTILS_H
