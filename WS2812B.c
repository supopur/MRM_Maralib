#include "WS2812B.h"
#include <string.h>

#define WS2812B_LEAD_SLOTS      1U
#define WS2812B_RESET_SLOTS     64U  // 64 * 1.25 us = 80 us reset pulse (> 50 us required)
#define WS2812B_BITS_PER_LED    24U
#define WS2812B_DMA_BUF_LEN     (WS2812B_LEAD_SLOTS + (WS2812B_NUM_LEDS * WS2812B_BITS_PER_LED) + WS2812B_RESET_SLOTS)

static TIM_HandleTypeDef *s_htim = NULL;
static uint32_t s_channel = TIM_CHANNEL_3;

static WS2812B_Color_t s_framebuffer[WS2812B_NUM_LEDS];
static uint8_t s_dma_buf[WS2812B_DMA_BUF_LEN];

static volatile bool s_dma_busy = false;
static uint8_t s_brightness = 100U;
static uint8_t s_thermal_throttle = 100U;
static bool s_overheat_shutdown = false;

static uint32_t s_last_current_ma = 0U;
static bool s_was_current_limited = false;

static void ws2812b_set_ccr(uint16_t val) {
    if (s_htim == NULL) {
        return;
    }
    __HAL_TIM_SET_COMPARE(s_htim, s_channel, val);
}

void WS2812B_Init(TIM_HandleTypeDef *htim, uint32_t channel) {
    s_htim = htim;
    s_channel = channel;
    s_dma_busy = false;
    s_brightness = 100U;
    s_thermal_throttle = 100U;
    s_overheat_shutdown = false;
    s_last_current_ma = WS2812B_NUM_LEDS * WS2812B_QUIESCENT_MA_PER_LED;
    s_was_current_limited = false;

    memset(s_framebuffer, 0, sizeof(s_framebuffer));
    memset(s_dma_buf, 0, sizeof(s_dma_buf));

    ws2812b_set_ccr(0);
    WS2812B_Show();
}

uint16_t WS2812B_XYToIndex(uint8_t x, uint8_t y) {
    if (x >= WS2812B_WIDTH || y >= WS2812B_HEIGHT) {
        return 0xFFFFU;
    }
#if WS2812B_SERPENTINE_ALTERNATING
    if ((x & 1U) != 0U) {
        return (uint16_t)x * WS2812B_HEIGHT + (WS2812B_HEIGHT - 1U - y);
    }
#endif
    return (uint16_t)x * WS2812B_HEIGHT + y;
}

void WS2812B_Clear(void) {
    memset(s_framebuffer, 0, sizeof(s_framebuffer));
}

void WS2812B_Fill(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < WS2812B_NUM_LEDS; i++) {
        s_framebuffer[i].r = r;
        s_framebuffer[i].g = g;
        s_framebuffer[i].b = b;
    }
}

void WS2812B_SetPixelIndex(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= WS2812B_NUM_LEDS) {
        return;
    }
    s_framebuffer[index].r = r;
    s_framebuffer[index].g = g;
    s_framebuffer[index].b = b;
}

void WS2812B_SetPixelXY(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) {
    uint16_t idx = WS2812B_XYToIndex(x, y);
    if (idx < WS2812B_NUM_LEDS) {
        s_framebuffer[idx].r = r;
        s_framebuffer[idx].g = g;
        s_framebuffer[idx].b = b;
    }
}

void WS2812B_SetBrightness(uint8_t percent) {
    s_brightness = (percent > 100U) ? 100U : percent;
}

uint8_t WS2812B_GetBrightness(void) {
    return s_brightness;
}

void WS2812B_SetThermalThrottle(uint8_t percent) {
    s_thermal_throttle = (percent > 100U) ? 100U : percent;
}

uint8_t WS2812B_GetThermalThrottle(void) {
    return s_thermal_throttle;
}

void WS2812B_SetOverheatShutdown(bool shutdown) {
    bool changed = (s_overheat_shutdown != shutdown);
    s_overheat_shutdown = shutdown;
    if (shutdown && changed && !s_dma_busy) {
        WS2812B_Show();
    }
}

bool WS2812B_GetOverheatShutdown(void) {
    return s_overheat_shutdown;
}

bool WS2812B_IsBusy(void) {
    return s_dma_busy;
}

uint32_t WS2812B_GetLastEstimatedCurrentMa(void) {
    return s_last_current_ma;
}

bool WS2812B_WasCurrentLimited(void) {
    return s_was_current_limited;
}

static void pack_byte_msb(uint8_t *dst, uint8_t byte) {
    for (uint8_t bit = 0; bit < 8U; bit++) {
        dst[bit] = (byte & (0x80U >> bit)) ? WS2812B_DUTY_1 : WS2812B_DUTY_0;
    }
}

bool WS2812B_Show(void) {
    if (s_htim == NULL || s_dma_busy) {
        return false;
    }

    uint32_t effective_pct = s_overheat_shutdown
                                 ? 0U
                                 : (((uint32_t)s_brightness * (uint32_t)s_thermal_throttle) / 100U);

    // 1. Estimate active current draw after brightness & thermal scaling
    uint32_t sum_rgb = 0U;
    for (uint16_t i = 0; i < WS2812B_NUM_LEDS; i++) {
        uint32_t r = ((uint32_t)s_framebuffer[i].r * effective_pct) / 100U;
        uint32_t g = ((uint32_t)s_framebuffer[i].g * effective_pct) / 100U;
        uint32_t b = ((uint32_t)s_framebuffer[i].b * effective_pct) / 100U;
        sum_rgb += (r + g + b);
    }

    const uint32_t quiescent_ma = WS2812B_NUM_LEDS * WS2812B_QUIESCENT_MA_PER_LED;
    uint32_t active_ma = (sum_rgb * WS2812B_MAX_CHANNEL_MA) / 255U;
    uint32_t max_active_ma = (WS2812B_MAX_CURRENT_MA > quiescent_ma)
                                 ? (WS2812B_MAX_CURRENT_MA - quiescent_ma)
                                 : 0U;

    uint32_t scale_q16 = 65536U;
    if (active_ma > max_active_ma && active_ma > 0U) {
        scale_q16 = (max_active_ma << 16) / active_ma;
        s_was_current_limited = true;
        s_last_current_ma = WS2812B_MAX_CURRENT_MA;
    } else {
        s_was_current_limited = false;
        s_last_current_ma = quiescent_ma + active_ma;
    }

    // 2. Pack DMA PWM duty buffer (GRB order for GL3528RGB01H-W / WS2812B)
    s_dma_buf[0] = 0U;
    uint32_t offset = WS2812B_LEAD_SLOTS;

    for (uint16_t i = 0; i < WS2812B_NUM_LEDS; i++) {
        uint8_t r = (uint8_t)((((uint32_t)s_framebuffer[i].r * effective_pct) / 100U * scale_q16) >> 16);
        uint8_t g = (uint8_t)((((uint32_t)s_framebuffer[i].g * effective_pct) / 100U * scale_q16) >> 16);
        uint8_t b = (uint8_t)((((uint32_t)s_framebuffer[i].b * effective_pct) / 100U * scale_q16) >> 16);

        pack_byte_msb(&s_dma_buf[offset], g);
        pack_byte_msb(&s_dma_buf[offset + 8U], r);
        pack_byte_msb(&s_dma_buf[offset + 16U], b);
        offset += WS2812B_BITS_PER_LED;
    }

    memset(&s_dma_buf[offset], 0, WS2812B_RESET_SLOTS);

    // 3. Start non-blocking PWM DMA transfer
    s_dma_busy = true;
    ws2812b_set_ccr(0);
    if (HAL_TIM_PWM_Start_DMA(s_htim, s_channel, (uint32_t *)s_dma_buf, WS2812B_DMA_BUF_LEN) != HAL_OK) {
        s_dma_busy = false;
        return false;
    }

    return true;
}

void WS2812B_OnDmaComplete(TIM_HandleTypeDef *htim) {
    if (s_htim == NULL || htim->Instance != s_htim->Instance) {
        return;
    }
    HAL_TIM_PWM_Stop_DMA(s_htim, s_channel);
    ws2812b_set_ccr(0);
    s_dma_busy = false;
}

void WS2812B_HSVtoRGB(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (s == 0U) {
        *r = v;
        *g = v;
        *b = v;
        return;
    }

    uint8_t region = h / 43U;
    uint8_t remainder = (uint8_t)((h - (region * 43U)) * 6U);

    uint8_t p = (uint8_t)(((uint16_t)v * (255U - s)) >> 8);
    uint8_t q = (uint8_t)(((uint16_t)v * (255U - (((uint16_t)s * remainder) >> 8))) >> 8);
    uint8_t t = (uint8_t)(((uint16_t)v * (255U - (((uint16_t)s * (255U - remainder)) >> 8))) >> 8);

    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

void WS2812B_TestPattern_Run(uint32_t now_ms) {
    typedef enum {
        TEST_STAGE_SNAKE = 0,
        TEST_STAGE_SOLID_COLORS,
        TEST_STAGE_RAINBOW
    } TestStage_t;

    static TestStage_t stage = TEST_STAGE_SNAKE;
    static uint32_t last_frame_ms = 0U;
    static uint32_t stage_start_ms = 0U;
    static uint16_t snake_head = 0U;
    static uint8_t snake_pass = 0U;
    static uint8_t rainbow_offset = 0U;

    if (WS2812B_IsBusy()) {
        return;
    }

    if (stage_start_ms == 0U) {
        stage_start_ms = now_ms;
    }

    switch (stage) {
        case TEST_STAGE_SNAKE: {
            if ((now_ms - last_frame_ms) < 20U) {
                return;
            }
            last_frame_ms = now_ms;

            WS2812B_Clear();
            const uint8_t tail_len = 12U;
            uint8_t base_r = (snake_pass == 0U) ? 255U : 0U;
            uint8_t base_g = (snake_pass == 1U) ? 255U : 0U;
            uint8_t base_b = (snake_pass == 2U) ? 255U : 0U;

            for (uint8_t t = 0U; t < tail_len; t++) {
                if (snake_head >= t) {
                    uint8_t scale = (uint8_t)(255U - (t * (255U / tail_len)));
                    WS2812B_SetPixelIndex((uint16_t)(snake_head - t),
                                          (uint8_t)(((uint16_t)base_r * scale) >> 8),
                                          (uint8_t)(((uint16_t)base_g * scale) >> 8),
                                          (uint8_t)(((uint16_t)base_b * scale) >> 8));
                }
            }
            WS2812B_Show();

            snake_head++;
            if (snake_head >= (WS2812B_NUM_LEDS + tail_len)) {
                snake_head = 0U;
                snake_pass++;
                if (snake_pass >= 3U) {
                    snake_pass = 0U;
                    stage = TEST_STAGE_SOLID_COLORS;
                    stage_start_ms = now_ms;
                }
            }
            break;
        }

        case TEST_STAGE_SOLID_COLORS: {
            if ((now_ms - last_frame_ms) < 100U) {
                return;
            }
            last_frame_ms = now_ms;

            uint32_t elapsed = now_ms - stage_start_ms;
            if (elapsed < 1500U) {
                WS2812B_Fill(255U, 0U, 0U);      // All Red
            } else if (elapsed < 3000U) {
                WS2812B_Fill(0U, 255U, 0U);      // All Green
            } else if (elapsed < 4500U) {
                WS2812B_Fill(0U, 0U, 255U);      // All Blue
            } else if (elapsed < 6000U) {
                WS2812B_Fill(255U, 255U, 255U);  // All White (clamped to 9.0 A by limiter)
            } else {
                stage = TEST_STAGE_RAINBOW;
                stage_start_ms = now_ms;
                return;
            }
            WS2812B_Show();
            break;
        }

        case TEST_STAGE_RAINBOW: {
            if ((now_ms - last_frame_ms) < 25U) {
                return;
            }
            last_frame_ms = now_ms;

            if ((now_ms - stage_start_ms) >= 6000U) {
                stage = TEST_STAGE_SNAKE;
                stage_start_ms = now_ms;
                snake_head = 0U;
                snake_pass = 0U;
                return;
            }

            for (uint8_t x = 0U; x < WS2812B_WIDTH; x++) {
                for (uint8_t y = 0U; y < WS2812B_HEIGHT; y++) {
                    uint8_t hue = (uint8_t)(rainbow_offset + (x * 9U) + (y * 12U));
                    uint8_t r, g, b;
                    WS2812B_HSVtoRGB(hue, 255U, 200U, &r, &g, &b);
                    WS2812B_SetPixelXY(x, y, r, g, b);
                }
            }
            rainbow_offset += 3U;
            WS2812B_Show();
            break;
        }
    }
}
