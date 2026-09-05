//
// Created by mat on 6/26/26.
//

#include "LightUtils.h"

#include "can.h"
#include "GlobalSync.h"
#include "tim.h"
#include "../../Src/PatternStorage.h"

#define NIGHT_MODE_BRIGHTNESS   ((uint16_t)(65535U * 40 / 100))   /* 40 % of full ARR */

static volatile uint8_t lights_active = 0;
static volatile uint8_t night_mode_active = 0;
static volatile uint8_t takedowns_active = 0;

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

uint8_t LightUtils_GetLights(void) {
    return lights_active;
}

void LightUtils_SetLights(uint8_t status) {
    lights_active = status;
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

static uint8_t prev_night_mode = 0;
static uint32_t lastSend = 0;
extern GlobalSyncHandle sync;

void LightUtils_Run(void) {
    /* ---- Night mode pattern switch ---- */
        if (night_mode_active != prev_night_mode)
        {
            prev_night_mode = night_mode_active;
            GlobalSync_SetPattern(&sync, night_mode_active ? &nightPattern : &normalPattern);
        }

        /* ---- CAN receive (polled fallback) ---- */
        if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0)
        {
            CAN_RxHeaderTypeDef header;
            uint8_t data[8];

            if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &header, data) == HAL_OK)
            {
                if (header.StdId == 0x010)
                {
                    uint32_t ts = ((uint32_t)data[0] << 24) |
                                  ((uint32_t)data[1] << 16) |
                                  ((uint32_t)data[2] << 8)  |
                                  ((uint32_t)data[3]);

                    GlobalSync_SetSyncPoint(&sync, ts, HAL_GetTick());
                }
                else if (data[0] == 0x01)
                {
                    lights_active = data[1];
                }
                else if (data[0] == 0x02)
                {
                    night_mode_active = data[1];
                }
                else if (data[0] == 0x03)
                {
                    takedowns_active = data[1] > 0;
                }
            }
        }

        /* ---- Drive outputs ---- */
        if (lights_active)
        {
            uint32_t ts = HAL_GetTick();

            for (uint8_t g = 0; g < MAX_PATTERN_GROUPS; g++)
            {
                const FlashStep_t *step = GlobalSync_GetGroupStep(&sync, ts, g);
                if (step == NULL)
                    continue;

                if (night_mode_active)
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
        if (takedowns_active)
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 65000);
        }
        else
        {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
        }
}