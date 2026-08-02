//
// Created by mat on 6/26/26.
//

#ifndef MAJAK_LIGHTUTILS_H
#define MAJAK_LIGHTUTILS_H

#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "Patterns.h"
#include "GPIO.h"

///@brief Convert a brightness percentage (0–100) to a 16-bit TIM ARR compare value.
///@param percentage Brightness in percent (0–MAX_BRIGHTNESS).
///@return ARR compare value (0–65535).
uint16_t BrightnessToARR(uint8_t percentage);

///@brief Convert a 16-bit TIM ARR compare value back to a brightness percentage.
///@param arr ARR compare value (0–65535).
///@return Brightness in percent (0–MAX_BRIGHTNESS).
uint8_t ARRToBrightness(uint16_t arr);

///@brief Drive all digital and PWM outputs of a FlashGroup according to the given step.
///       Reads active/intensity from the step and output lists from the group directly.
///       - Digital outputs: active == true → HIGH, false → LOW.
///         If intensity > 0 it takes precedence and is treated as active.
///       - PWM outputs: intensity used directly as ARR compare value;
///         falls back to active ? 65535 : 0 when intensity == 0.
///@param group Pointer to the FlashGroup_t containing output descriptors.
///@param step  Pointer to the current FlashStep_t to apply.
void LightUtils_DriveGroup(const FlashGroup_t *group, const FlashStep_t *step);

#endif // MAJAK_LIGHTUTILS_H