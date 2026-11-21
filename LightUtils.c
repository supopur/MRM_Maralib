//
// Created by mat on 6/26/26.
//

#include "LightUtils.h"
#include "ProductConfig.h"

#include <string.h>
#include "GlobalSync.h"
#include "MaraLib/CANProtocol.h"

#if FEATURE_SOFT_PWM
#include "SoftPWM.h"
#endif

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
    uint8_t takedown_id;
    ChannelDriverType_t driver_type;
    uint8_t light_type;
    bool active_high;
    TIM_HandleTypeDef *pwm_timer;
    uint32_t pwm_channel;
    GPIO_TypeDef *gpio_port;
    uint16_t gpio_pin;
    uint8_t soft_pwm_index;
} LightChannel_t;

static LightChannel_t registeredChannels[MAX_LIGHT_CHANNELS];
static uint8_t registeredChannelCount = 0;

void LightUtils_ClearChannels(void) {
    memset(registeredChannels, 0, sizeof(registeredChannels));
    registeredChannelCount = 0;
}

void LightUtils_RegisterPwmChannelEx(uint8_t flash_channel_id, uint8_t takedown_id, uint8_t light_type, TIM_HandleTypeDef *timer, uint32_t tim_channel) {
    if (registeredChannelCount >= MAX_LIGHT_CHANNELS) return;
    LightChannel_t *ch = &registeredChannels[registeredChannelCount++];
    ch->channel_id = flash_channel_id;
    ch->takedown_id = takedown_id;
    ch->driver_type = CHANNEL_OUTPUT_PWM;
    ch->light_type = light_type;
    ch->active_high = true;
    ch->pwm_timer = timer;
    ch->pwm_channel = tim_channel;
}

void LightUtils_RegisterPwmChannel(uint8_t channel_id, uint8_t light_type, TIM_HandleTypeDef *timer, uint32_t tim_channel) {
    LightUtils_RegisterPwmChannelEx(channel_id, channel_id, light_type, timer, tim_channel);
}

void LightUtils_RegisterGpioChannelEx(uint8_t flash_channel_id, uint8_t takedown_id, uint8_t light_type, GPIO_TypeDef *port, uint16_t pin, bool active_high) {
    if (registeredChannelCount >= MAX_LIGHT_CHANNELS) return;
    LightChannel_t *ch = &registeredChannels[registeredChannelCount++];
    ch->channel_id = flash_channel_id;
    ch->takedown_id = takedown_id;
    ch->driver_type = CHANNEL_OUTPUT_GPIO;
    ch->light_type = light_type;
    ch->active_high = active_high;
    ch->gpio_port = port;
    ch->gpio_pin = pin;
}

void LightUtils_RegisterGpioChannel(uint8_t channel_id, uint8_t light_type, GPIO_TypeDef *port, uint16_t pin, bool active_high) {
    LightUtils_RegisterGpioChannelEx(channel_id, channel_id, light_type, port, pin, active_high);
}

void LightUtils_RegisterSoftPwmChannelEx(uint8_t flash_channel_id, uint8_t takedown_id, uint8_t light_type, uint8_t soft_pwm_index) {
#if FEATURE_SOFT_PWM
    if (registeredChannelCount >= MAX_LIGHT_CHANNELS) return;
    LightChannel_t *ch = &registeredChannels[registeredChannelCount++];
    ch->channel_id = flash_channel_id;
    ch->takedown_id = takedown_id;
    ch->driver_type = CHANNEL_OUTPUT_SOFT_PWM;
    ch->light_type = light_type;
    ch->soft_pwm_index = soft_pwm_index;
#else
    (void)flash_channel_id; (void)takedown_id; (void)light_type; (void)soft_pwm_index;
#endif
}

void LightUtils_RegisterSoftPwmChannel(uint8_t channel_id, uint8_t light_type, uint8_t soft_pwm_index) {
    LightUtils_RegisterSoftPwmChannelEx(channel_id, channel_id, light_type, soft_pwm_index);
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
    bool runEmergencyLights = lightsActive && !overheatShutdown;

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

    const PatternStep_t *currentStep = runEmergencyLights ? GlobalSync_GetCurrentStep(&sync, ts) : NULL;
    uint16_t elapsedInStep = (currentStep != NULL) ? GlobalSync_GetElapsedInStep(&sync, ts) : 0;

    for (uint8_t i = 0; i < registeredChannelCount; i++) {
        LightChannel_t *ch = &registeredChannels[i];
        if (ch->driver_type == CHANNEL_OUTPUT_NONE) continue;

        bool isChannelActive = false;
        bool isCruise = false;
        uint32_t intensity = 0;

        // Check night mode flash shortening for traditional square-wave flash steps on this channel
        bool channelNightOff = false;
        if (runEmergencyLights && nightModeActive && currentStep != NULL && ch->channel_id < MAX_PATTERN_CHANNELS) {
            const ChannelKeyframe_t *kf = &currentStep->channels[ch->channel_id];
            if (kf->fade_in_units == 0 && kf->fade_out_units == 0) {
                uint16_t nightActiveDuration = currentStep->duration_ms / NIGHT_STEP_DIVIDER;
                if (nightActiveDuration == 0 && currentStep->duration_ms > 0) {
                    nightActiveDuration = 1;
                }
                if (elapsedInStep >= nightActiveDuration) {
                    channelNightOff = true;
                }
            }
        }

        if (overheatShutdown) {
            intensity = 0;
            isChannelActive = false;
        } else if (ch->light_type == LIGHT_TYPE_TAKEDOWNS && isTakedownOn && ((takedownMask & (1UL << ch->takedown_id)) != 0)) {
            // Steady takedown mode takes highest priority for takedown channels
            isChannelActive = true;
            intensity = effTakedown;
        } else if (ch->light_type == LIGHT_TYPE_TAKEDOWNS) {
            // If steady takedown is not active on this channel, allow flash patterns to flash it
            if (runEmergencyLights && !channelNightOff) {
                uint8_t b = GlobalSync_GetChannelBrightness(&sync, ch->channel_id, ts);
                if (b > 0) {
                    isChannelActive = true;
                    intensity = ((uint32_t)effTakedown * b) / MAX_BRIGHTNESS;
                }
            }
        } else {
            // Standard emergency warning light channels
            if (runEmergencyLights) {
                uint8_t b = channelNightOff ? 0 : GlobalSync_GetChannelBrightness(&sync, ch->channel_id, ts);
                if (b > 0) {
                    isChannelActive = true;
                    intensity = ((uint32_t)effEmergency * b) / MAX_BRIGHTNESS;
                } else if (cruiseModeActive) {
                    isCruise = true;
                    intensity = effCruise;
                }
            } else if (cruiseModeActive) {
                isCruise = true;
                intensity = effCruise;
            }
        }

        if (ch->driver_type == CHANNEL_OUTPUT_PWM) {
            if (ch->pwm_timer != NULL) {
                __HAL_TIM_SET_COMPARE(ch->pwm_timer, ch->pwm_channel, (uint16_t)intensity);
            }
        } else if (ch->driver_type == CHANNEL_OUTPUT_SOFT_PWM) {
#if FEATURE_SOFT_PWM
            SoftPWM_SetDuty16(ch->soft_pwm_index, (uint16_t)intensity);
#endif
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
