#ifndef MARALIB_WS2812B_H
#define MARALIB_WS2812B_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Matrix geometry (7x28 = 196 LEDs)
#ifndef WS2812B_WIDTH
#define WS2812B_WIDTH                   28U
#endif

#ifndef WS2812B_HEIGHT
#define WS2812B_HEIGHT                  7U
#endif

#define WS2812B_NUM_LEDS                (WS2812B_WIDTH * WS2812B_HEIGHT)

// Wiring order:
// 0 = Every column runs Top -> Down (bottom DOUT of col X -> top DIN of col X+1)
// 1 = Alternating serpentine (even cols Top -> Down, odd cols Bottom -> Up)
#ifndef WS2812B_SERPENTINE_ALTERNATING
#define WS2812B_SERPENTINE_ALTERNATING  0
#endif

// TIM1 @ 72 MHz -> ARR = 89 gives 800 kHz bit clock (1.25 us per bit)
#ifndef WS2812B_DUTY_0
#define WS2812B_DUTY_0                  29U   // ~0.40 us high
#endif

#ifndef WS2812B_DUTY_1
#define WS2812B_DUTY_1                  58U   // ~0.81 us high
#endif

// Hardware current limit for SIC471ED-T1-GE3 (12 A continuous buck -> 9.0 A software cap)
#ifndef WS2812B_MAX_CURRENT_MA
#define WS2812B_MAX_CURRENT_MA          9000U
#endif

#ifndef WS2812B_QUIESCENT_MA_PER_LED
#define WS2812B_QUIESCENT_MA_PER_LED    1U
#endif

#ifndef WS2812B_MAX_CHANNEL_MA
#define WS2812B_MAX_CHANNEL_MA          20U
#endif

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} WS2812B_Color_t;

/// @brief Initialize the WS2812B driver on the given timer & channel (must have DMA configured).
void WS2812B_Init(TIM_HandleTypeDef *htim, uint32_t channel);

/// @brief Convert (x, y) matrix coordinates (0..WIDTH-1, 0..HEIGHT-1) to physical LED index.
uint16_t WS2812B_XYToIndex(uint8_t x, uint8_t y);

/// @brief Clear all pixels to black (off) in the framebuffer.
void WS2812B_Clear(void);

/// @brief Fill all pixels in the framebuffer with a single RGB color.
void WS2812B_Fill(uint8_t r, uint8_t g, uint8_t b);

/// @brief Set pixel by physical chain index (0 .. WS2812B_NUM_LEDS - 1).
void WS2812B_SetPixelIndex(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

/// @brief Set pixel by 2D matrix coordinate (x: 0..27 left-to-right, y: 0..6 top-to-bottom).
void WS2812B_SetPixelXY(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);

/// @brief Set global user brightness percentage (0..100).
void WS2812B_SetBrightness(uint8_t percent);

/// @brief Get global user brightness percentage (0..100).
uint8_t WS2812B_GetBrightness(void);

/// @brief Set thermal throttling factor (0..100), typically driven by NTC ADC logic.
void WS2812B_SetThermalThrottle(uint8_t percent);

/// @brief Get current thermal throttling factor (0..100).
uint8_t WS2812B_GetThermalThrottle(void);

/// @brief Enable or disable overheat shutdown (forces all LEDs off immediately).
void WS2812B_SetOverheatShutdown(bool shutdown);

/// @brief Check if overheat shutdown is currently active.
bool WS2812B_GetOverheatShutdown(void);

/// @brief Returns true if a DMA frame transfer is currently in progress.
bool WS2812B_IsBusy(void);

/// @brief Apply brightness, thermal throttle, and 9 A current limit, then start DMA transfer.
/// @return true if transfer started, false if DMA was busy or driver uninitialized.
bool WS2812B_Show(void);

/// @brief Call from HAL_TIM_PWM_PulseFinishedCallback() to cleanly terminate DMA transfer.
void WS2812B_OnDmaComplete(TIM_HandleTypeDef *htim);

/// @brief Get estimated total current draw (mA) of the last transmitted frame.
uint32_t WS2812B_GetLastEstimatedCurrentMa(void);

/// @brief Returns true if the last frame was scaled down by the 9 A software current limiter.
bool WS2812B_WasCurrentLimited(void);

/// @brief Convert 8-bit HSV (h: 0..255, s: 0..255, v: 0..255) to RGB.
void WS2812B_HSVtoRGB(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);

/// @brief Run non-blocking built-in test pattern (Snake -> Solid R/G/B/W -> 2D Rainbow).
void WS2812B_TestPattern_Run(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif // MARALIB_WS2812B_H
