//
// Created by mat on 6/26/26.
//

#ifndef MAJAK_LIGHTUTILS_H
#define MAJAK_LIGHTUTILS_H

#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "Patterns.h"

#define MAX_LIGHT_CHANNELS 32
#define NIGHT_STEP_DIVIDER 4

///@brief Output driver type for a channel
typedef enum {
    CHANNEL_OUTPUT_NONE = 0,
    CHANNEL_OUTPUT_PWM,
    CHANNEL_OUTPUT_GPIO
} ChannelDriverType_t;

///@brief Register a PWM-driven output channel.
///@param channel_id Logical channel index (0 to 31) matching bit in output_mask.
///@param light_type LIGHT_TYPE_EMERGENCY or LIGHT_TYPE_TAKEDOWNS.
///@param timer Pointer to STM32 HAL TIM handle.
///@param tim_channel Timer channel (e.g. TIM_CHANNEL_1).
void LightUtils_RegisterPwmChannel(uint8_t channel_id, uint8_t light_type, TIM_HandleTypeDef *timer, uint32_t tim_channel);

///@brief Register a GPIO-driven output channel.
///@param channel_id Logical channel index (0 to 31) matching bit in output_mask.
///@param light_type LIGHT_TYPE_EMERGENCY or LIGHT_TYPE_TAKEDOWNS.
///@param port Pointer to GPIO port.
///@param pin Pin mask.
///@param active_high Polarity (true = active high, false = active low).
void LightUtils_RegisterGpioChannel(uint8_t channel_id, uint8_t light_type, GPIO_TypeDef *port, uint16_t pin, bool active_high);

///@brief Clear all registered channel mappings.
void LightUtils_ClearChannels(void);

///@brief Convert brightness percentage (0–MAX_BRIGHTNESS) to 16-bit TIM ARR compare value.
uint16_t BrightnessToARR(uint8_t percentage);

///@brief Convert 16-bit TIM ARR compare value to brightness percentage (0–MAX_BRIGHTNESS).
uint8_t ARRToBrightness(uint16_t arr);

///@brief Get whether flashing lights are active.
bool LightUtils_GetLights(void);

///@brief Set flashing lights active state.
void LightUtils_SetLights(bool status);

///@brief Get night mode active state.
bool LightUtils_GetNightMode(void);

///@brief Set night mode active state.
void LightUtils_SetNightMode(bool status);

///@brief Get cruise / steady burn mode active state.
bool LightUtils_GetCruiseMode(void);

///@brief Set cruise / steady burn mode active state.
void LightUtils_SetCruiseMode(bool status);

///@brief Get combined light status bitmask (bit 0: flashing, bit 1: night, bit 2: cruise).
uint8_t LightUtils_GetLightStatus(void);

///@brief Set light status from bitmask (bit 0: flashing, bit 1: night, bit 2: cruise).
void LightUtils_SetLightStatus(uint8_t statusBitmask);

///@brief Get takedowns active state.
bool LightUtils_GetTakedowns(void);

///@brief Set takedowns active state for all channels.
void LightUtils_SetTakedowns(bool status);

///@brief Set takedowns active state with channel bitmask.
///@param status Active state (true = on, false = off).
///@param mask Bitmask of takedown channels to enable (Bit N enables Takedown Channel N).
void LightUtils_SetTakedownsWithMask(bool status, uint32_t mask);

///@brief Get takedown active channel bitmask.
uint32_t LightUtils_GetTakedownMask(void);

///@brief Get brightness percentage (0-100) for a specific light type.
///@param targetType LIGHT_TYPE_EMERGENCY, LIGHT_TYPE_TAKEDOWNS, or LIGHT_TYPE_NIGHT.
uint8_t LightUtils_GetBrightness(uint8_t targetType);

///@brief Set brightness percentage (0-100) for a specific light type.
///@param targetType LIGHT_TYPE_EMERGENCY, LIGHT_TYPE_TAKEDOWNS, LIGHT_TYPE_NIGHT, or LIGHT_TYPE_ALL.
///@param brightness Value from 0 to 100.
void LightUtils_SetBrightness(uint8_t targetType, uint8_t brightness);

///@brief Set thermal throttling percentage (0-100).
void LightUtils_SetThermalThrottle(uint8_t throttlePercent);

///@brief Get thermal throttling percentage (0-100).
uint8_t LightUtils_GetThermalThrottle(void);

///@brief Set overheat shutdown flag.
void LightUtils_SetOverheatShutdown(bool shutdown);

///@brief Get overheat shutdown flag.
bool LightUtils_GetOverheatShutdown(void);

///@brief Get current active pattern ID.
uint8_t LightUtils_GetActivePatternId(void);

///@brief Set current active pattern ID.
void LightUtils_SetActivePatternId(uint8_t id);

///@brief Main periodic light controller task.
void LightUtils_Run(void);
#endif // MAJAK_LIGHTUTILS_H