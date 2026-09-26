//
// Created by mat on 6/26/26.
//

#include "LightUtils.h"

#include <string.h>
#include "GlobalSync.h"
#include "MaraLib/CANProtocol.h"

#define CRUISE_MODE_PERCENT 5

static volatile bool lightsActive = false;
static volatile bool nightModeActive = false;
static volatile bool cruiseModeActive = false;
static volatile bool takedownsActive = false;
static volatile uint32_t takedownMask = 0;

static volatile uint8_t emergencyBrightness = MAX_BRIGHTNESS;
static volatile uint8_t takedownBrightness = MAX_BRIGHTNESS;
static volatile uint8_t nightBrightness = 40;

static volatile uint8_t thermalThrottlePercent = 100;
static volatile bool overheatShutdown = false;

typedef struct {
    uint8_t channel_id;
    ChannelDriverType_t driver_type;
    uint8_t light_type;
    bool active_high;
    TIM_HandleTypeDef *pwm_timer;
    uint32_t pwm_channel;
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
} LightChannel_t;

static LightChannel_t registeredChannels[MAX_LIGHT_CHANNELS];
static uint8_t registeredChannelCount = 0;

void LightUtils_ClearChannels(void) {
    memset(registeredChannels, 0, sizeof(registeredChannels));
    registeredChannelCount = 0;
}

void LightUtils_RegisterPwmChannel(uint8_t channel_id, uint8_t light_type, TIM_HandleTypeDef *timer, uint32_t tim_channel) {
    if (registeredChannelCount >= MAX_LIGHT_CHANNELS) return;
    LightChannel_t *ch = &registeredChannels[registeredChannelCount++];
    ch->channel_id = channel_id;
    ch->driver_type = CHANNEL_OUTPUT_PWM;
    ch->light_type = light_type;
    ch->active_high = true;
    ch->pwm_timer = timer;
    ch->pwm_channel = tim_channel;
}

void LightUtils_RegisterGpioChannel(uint8_t channel_id, uint8_t light_type, GPIO_TypeDef *port, uint16_t pin, bool active_high) {
    if (registeredChannelCount >= MAX_LIGHT_CHANNELS) return;
    LightChannel_t *ch = &registeredChannels[registeredChannelCount++];
    ch->channel_id = channel_id;
    ch->driver_type = CHANNEL_OUTPUT_GPIO;
    ch->light_type = light_type;
    ch->active_high = active_high;
    ch->gpio_port = port;
    ch->gpio_pin = pin;
}

uint16_t BrightnessToARR(uint8_t percentage) {
    if (percentage > MAX_BRIGHTNESS) percentage = MAX_BRIGHTNESS;
    return (uint16_t)((uint32_t)percentage * 65535U / MAX_BRIGHTNESS);
}

uint8_t ARRToBrightness(uint16_t arr) {
    return (uint8_t)((uint32_t)arr * MAX_BRIGHTNESS / 65535U);
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

bool LightUtils_GetCruiseMode(void) {
    return cruiseModeActive;
}

void LightUtils_SetCruiseMode(bool status) {
    cruiseModeActive = status;
}

uint8_t LightUtils_GetLightStatus(void) {
    uint8_t status = 0;
    if (lightsActive) status |= LIGHT_FLAG_FLASHING;
    if (nightModeActive) status |= LIGHT_FLAG_NIGHT;
    if (cruiseModeActive) status |= LIGHT_FLAG_CRUISE;
    return status;
}

void LightUtils_SetLightStatus(uint8_t statusBitmask) {
    lightsActive = (statusBitmask & LIGHT_FLAG_FLASHING) != 0;
    nightModeActive = (statusBitmask & LIGHT_FLAG_NIGHT) != 0;
    cruiseModeActive = (statusBitmask & LIGHT_FLAG_CRUISE) != 0;
}

bool LightUtils_GetTakedowns(void) {
    return takedownsActive;
}

void LightUtils_SetTakedowns(bool status) {
    takedownsActive = status;
    takedownMask = status ? 0xFFFFFFFF : 0;
}

void LightUtils_SetTakedownsWithMask(bool status, uint32_t mask) {
    takedownsActive = status;
    takedownMask = status ? (mask == 0 ? 0xFFFFFFFF : mask) : 0;
}

uint32_t LightUtils_GetTakedownMask(void) {
    return takedownMask;
}

uint8_t LightUtils_GetBrightness(uint8_t targetType) {
    switch (targetType) {
        case LIGHT_TYPE_TAKEDOWNS:
            return takedownBrightness;
        case LIGHT_TYPE_NIGHT:
            return nightBrightness;
        case LIGHT_TYPE_EMERGENCY:
        case LIGHT_TYPE_ALL:
        default:
            return emergencyBrightness;
    }
}

void LightUtils_SetBrightness(uint8_t targetType, uint8_t brightness) {
    if (brightness > MAX_BRIGHTNESS) {
        brightness = MAX_BRIGHTNESS;
    }
    switch (targetType) {
        case LIGHT_TYPE_EMERGENCY:
            emergencyBrightness = brightness;
            break;
        case LIGHT_TYPE_TAKEDOWNS:
            takedownBrightness = brightness;
            break;
        case LIGHT_TYPE_NIGHT:
            nightBrightness = brightness;
            break;
        case LIGHT_TYPE_ALL:
            emergencyBrightness = brightness;
            takedownBrightness = brightness;
            nightBrightness = brightness;
            break;
        default:
            emergencyBrightness = brightness;
            break;
    }
}

void LightUtils_SetThermalThrottle(uint8_t throttlePercent) {
    thermalThrottlePercent = throttlePercent;
}

uint8_t LightUtils_GetThermalThrottle(void) {
    return thermalThrottlePercent;
}

void LightUtils_SetOverheatShutdown(bool shutdown) {
    overheatShutdown = shutdown;
}

bool LightUtils_GetOverheatShutdown(void) {
    return overheatShutdown;
}

uint8_t LightUtils_GetActivePatternId(void) {
    return activePatternId;
}

void LightUtils_SetActivePatternId(uint8_t id) {
    activePatternId = id;
}

extern GlobalSyncHandle sync;

void LightUtils_Run(void) {
    uint32_t ts = HAL_GetTick();

    bool isTakedownOn = takedownsActive && (takedownMask != 0) && !overheatShutdown;

    // takedowns override emergency lights: emergency lights are suppressed while takedowns are active
    bool runEmergencyLights = lightsActive && !isTakedownOn && !overheatShutdown;

    uint32_t activeMask = 0;
    if (runEmergencyLights) {
        const PatternStep_t *step = GlobalSync_GetCurrentStep(&sync, ts);
        if (step != NULL) {
            if (nightModeActive) {
                // shorten active flash duration by dividing by configured divisor
                uint16_t timeRemaining = GlobalSync_GetTimeRemainingInStep(&sync, ts);
                uint16_t elapsedInStep = (step->duration_ms > timeRemaining) ? (step->duration_ms - timeRemaining) : 0;
                uint16_t nightActiveDuration = step->duration_ms / NIGHT_STEP_DIVIDER;
                if (nightActiveDuration == 0 && step->duration_ms > 0) {
                    nightActiveDuration = 1;
                }

                if (elapsedInStep < nightActiveDuration) {
                    activeMask = step->output_mask;
                } else {
                    // fill remaining step time with waiting while keeping total step length identical
                    activeMask = 0;
                }
            } else {
                activeMask = step->output_mask;
            }
        }
    }

    // calculate effective emergency pwm compare value
    uint8_t currentEmergencyBright = nightModeActive ? nightBrightness : emergencyBrightness;
    uint32_t effEmergency = (uint32_t)65535U * currentEmergencyBright / MAX_BRIGHTNESS * thermalThrottlePercent / 100U;
    if (effEmergency > 65535U) effEmergency = 65535U;

    // calculate cruise mode compare value for steady burn
    uint32_t effCruise = (uint32_t)65535U * CRUISE_MODE_PERCENT / MAX_BRIGHTNESS * thermalThrottlePercent / 100U;
    if (effCruise > 65535U) effCruise = 65535U;

    // calculate takedowns compare value
    uint32_t effTakedown = (uint32_t)65000U * takedownBrightness / MAX_BRIGHTNESS * thermalThrottlePercent / 100U;
    if (effTakedown > 65535U) effTakedown = 65535U;

    // iterate all hardware channels configured by firmware
    for (uint8_t i = 0; i < registeredChannelCount; i++) {
        LightChannel_t *ch = &registeredChannels[i];
        if (ch->driver_type == CHANNEL_OUTPUT_NONE) continue;

        bool isChannelActive = false;
        bool isCruise = false;
        uint32_t intensity = 0;

        if (overheatShutdown) {
            intensity = 0;
            isChannelActive = false;
        } else if (ch->light_type == LIGHT_TYPE_TAKEDOWNS) {
            // takedown channels are independently driven by their takedown channel id bit
            if (isTakedownOn && ((takedownMask & (1UL << ch->channel_id)) != 0)) {
                isChannelActive = true;
                intensity = effTakedown;
            }
        } else {
            // emergency warning light channel: suppressed when takedowns are on
            if (runEmergencyLights) {
                if ((activeMask & (1UL << ch->channel_id)) != 0) {
                    isChannelActive = true;
                    intensity = effEmergency;
                } else if (cruiseModeActive) {
                    isCruise = true;
                    intensity = effCruise;
                }
            } else if (!isTakedownOn && cruiseModeActive) {
                isCruise = true;
                intensity = effCruise;
            }
        }

        if (ch->driver_type == CHANNEL_OUTPUT_PWM) {
            if (ch->pwm_timer != NULL) {
                __HAL_TIM_SET_COMPARE(ch->pwm_timer, ch->pwm_channel, (uint16_t)intensity);
            }
        } else if (ch->driver_type == CHANNEL_OUTPUT_GPIO) {
            if (ch->gpio_port != NULL) {
                bool pinActive = isChannelActive || isCruise;
                if (!ch->active_high) {
                    pinActive = !pinActive;
                }
                HAL_GPIO_WritePin(ch->gpio_port, ch->gpio_pin, pinActive ? GPIO_PIN_SET : GPIO_PIN_RESET);
            }
        }
    }
}