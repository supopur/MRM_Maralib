//
// Created by mat on 6/26/26.
//

#include "LightUtils.h"

uint16_t BrightnessToARR(uint8_t percentage) {
    return (uint16_t)((uint32_t)percentage * 65535U / MAX_BRIGHTNESS);
}

uint8_t ARRToBrightness(uint16_t arr) {
    return (uint8_t)((uint32_t)arr * MAX_BRIGHTNESS / 65535U);
}

void LightUtils_DriveGroup(const FlashGroup_t *group, const FlashStep_t *step) {
    // intensity > 0 overrides active for both output types
    bool     digital_on  = step->intensity > 0 ? true : step->active;
    uint16_t pwm_compare = step->intensity > 0 ? step->intensity
                                                : (step->active ? 65535U : 0U);

    // Drive digital outputs
    for (uint8_t i = 0; i < MAX_PATTERN_FLASH_GROUP_OUTPUTS; i++) {
        if (group->digitalOutputs[i].outputPort == NULL) break;
        HAL_GPIO_WritePin(group->digitalOutputs[i].outputPort,
                          group->digitalOutputs[i].outputPin,
                          digital_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    // Drive PWM outputs
    for (uint8_t i = 0; i < MAX_PATTERN_FLASH_GROUP_OUTPUTS; i++) {
        if (group->pwmOutputs[i].pwmTimer == NULL) break;
        __HAL_TIM_SET_COMPARE(group->pwmOutputs[i].pwmTimer,
                              group->pwmOutputs[i].pwmChannel,
                              pwm_compare);
    }
}