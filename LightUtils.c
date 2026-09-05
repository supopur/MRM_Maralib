//
// Created by mat on 6/26/26.
//

#include "LightUtils.h"

#include "can.h"
#include "GlobalSync.h"
#include "tim.h"
#include "../../Src/PatternStorage.h"

#define NIGHT_MODE_BRIGHTNESS   ((uint16_t)(65535U * 40 / 100))   /* 40 % of full ARR */

static volatile bool lightsActive = false;
static volatile bool nightModeActive = false;
static volatile bool takedownsActive = false;

uint16_t BrightnessToARR(uint8_t percentage) {
    return (uint16_t)((uint32_t)percentage * 65535U / MAX_BRIGHTNESS);
}

uint8_t ARRToBrightness(uint16_t arr) {
    return (uint8_t)((uint32_t)arr * MAX_BRIGHTNESS / 65535U);
}

void LightUtils_DriveGroup(const FlashGroup_t *group, const FlashStep_t *step) {
    // Apply inversion for digital output — NPN drivers are active-low
    GPIO_PinState digital_state = (step->active ^ group->inverted) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    // Drive digital outputs
    for (uint8_t i = 0; i < MAX_PATTERN_FLASH_GROUP_OUTPUTS; i++) {
        if (group->digitalOutputs[i].outputPort == NULL) break;
        HAL_GPIO_WritePin(group->digitalOutputs[i].outputPort,
                          group->digitalOutputs[i].outputPin,
                          digital_state);
    }

    // Drive PWM outputs (inversion not applied — PWM polarity is set in CubeMX)
    for (uint8_t i = 0; i < MAX_PATTERN_FLASH_GROUP_OUTPUTS; i++) {
        if (group->pwmOutputs[i].pwmTimer == NULL) break;
        __HAL_TIM_SET_COMPARE(group->pwmOutputs[i].pwmTimer,
                              group->pwmOutputs[i].pwmChannel,
                              step->intensity);
    }
}

void LightUtils_DriveGroupOff(const FlashGroup_t *group) {
    // Idle state: active=false, intensity=0 — but inverted groups must go HIGH
    FlashStep_t offStep = { .dwell = 0, .active = false, .intensity = 0 };
    LightUtils_DriveGroup(group, &offStep);
}

bool LightUtils_GetLights(void) {
    return lightsActive;
}

void LightUtils_SetLights(bool status) {
    lightsActive = status;
}

bool LightUtils_GetNightMode(void) {
    return nightModeActive;
}

void LightUtils_SetNightMode(bool status) {
    nightModeActive = status;
}

bool LightUtils_GetTakedowns(void) {
    return takedownsActive;
}

void LightUtils_SetTakedowns(bool status) {
    takedownsActive = status;
}

/**
 * @brief  Drive a group at 40 % brightness.
 *         Builds a temporary step with intensity pre-scaled and delegates
 *         to LightUtils_DriveGroup so the PWM output loop is handled normally.
 */
static void DriveGroupNightMode(const FlashGroup_t *group, const FlashStep_t *step) {
    bool want_on = (step->intensity > 0) ? true : step->active;

    FlashStep_t nightStep = *step;
    nightStep.intensity = want_on ? NIGHT_MODE_BRIGHTNESS : 0U;
    nightStep.active = false;

    LightUtils_DriveGroup(group, &nightStep);
}

static bool prevNightMode = false;
static uint32_t lastSend = 0;
extern GlobalSyncHandle sync;

void LightUtils_Run(void) {
    /* ---- Night mode pattern switch ---- */
        if (nightModeActive != prevNightMode)
        {
            prevNightMode = nightModeActive;
            GlobalSync_SetPattern(&sync, nightModeActive ? &nightPattern : &normalPattern);
        }

        /* ---- Drive outputs ---- */
        if (lightsActive)
        {
            uint32_t ts = HAL_GetTick();

            for (uint8_t g = 0; g < MAX_PATTERN_GROUPS; g++)
            {
                const FlashStep_t *step = GlobalSync_GetGroupStep(&sync, ts, g);
                if (step == NULL)
                    continue;

                if (nightModeActive)
                    DriveGroupNightMode(&normalPattern.groups[g], step);
                else
                    LightUtils_DriveGroup(&normalPattern.groups[g], step);
            }
        }
        else
        {
            for (uint8_t g = 0; g < MAX_PATTERN_GROUPS; g++)
                LightUtils_DriveGroupOff(&normalPattern.groups[g]);
        }

        /* ---- Takedowns ---- */
        if (takedownsActive)
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 65000);
        }
        else
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
        }
}