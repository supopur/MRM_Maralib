//
// Created by mat on 6/26/26.
//

#ifndef MAJAK_LIGHTUTILS_H
#define MAJAK_LIGHTUTILS_H

#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "Patterns.h"

///@brief Convert brightness percentage (0–MAX_BRIGHTNESS) to 16-bit TIM ARR compare value.
uint16_t BrightnessToARR(uint8_t percentage);

///@brief Convert 16-bit TIM ARR compare value to brightness percentage (0–MAX_BRIGHTNESS).
uint8_t ARRToBrightness(uint16_t arr);

///@brief Drive all outputs of a FlashGroup according to the given step.
///       Respects group->inverted: active-low groups have their GPIO state flipped.
///       PWM polarity is assumed to be configured in CubeMX, inversion not applied.
///@param group Pointer to the group (contains output descriptors and inverted flag).
///@param step  Pointer to the step to apply.
void LightUtils_DriveGroup(const FlashGroup_t *group, const FlashStep_t *step);

///@brief Drive a group to its idle/off state, respecting inversion.
///       Inverted groups → all outputs HIGH. Normal groups → all outputs LOW.
///@param group Pointer to the group to blank.
void LightUtils_DriveGroupOff(const FlashGroup_t *group);

bool LightUtils_GetLights(void);
void LightUtils_SetLights(bool status);

bool LightUtils_GetNightMode(void);
void LightUtils_SetNightMode(bool status);

bool LightUtils_GetTakedowns(void);
void LightUtils_SetTakedowns(bool status);

uint8_t LightUtils_GetBrightness(void);
void LightUtils_SetBrightness(uint8_t brightness);

void LightUtils_Run(void);
#endif // MAJAK_LIGHTUTILS_H