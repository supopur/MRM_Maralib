//
// Created by mat on 6/26/26.
//

#include "LightUtils.h"

uint16_t PercentageToARR(float percentage) {
    return (uint16_t)(percentage / 100.0f * 65535.0f);
}

uint8_t ARRToPercentage(uint16_t arr) {
    return (uint8_t)((arr * 100U) / 65535U);
}

// not in degrees but in raw ADC values
static uint16_t targetReading = 0;
static uint16_t currentReading = 0;

uint16_t SetPWM(uint16_t dutyCycle, uint32_t Channel, TIM_HandleTypeDef* tim) {
    int32_t correction = (int32_t)P_CONTROLLER_GAIN * ((int32_t)targetReading - (int32_t)currentReading);
    int32_t output = (int32_t)dutyCycle - correction;
    if (output < 0) output = 0;
    if (output > (int32_t)dutyCycle) output = (int32_t)dutyCycle;
    __HAL_TIM_SET_COMPARE(tim, Channel, (uint16_t)output);
    return (uint16_t)output;
}

void UpdateCurrentTemp(uint16_t currentTempReading, uint16_t targetTempReading) {
    currentReading = currentTempReading;
    targetReading = targetTempReading;
}
