#ifndef MARALIB_MATRIXTEXT_H
#define MARALIB_MATRIXTEXT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MATRIX_ALIGN_LEFT = 0,
    MATRIX_ALIGN_CENTER,
    MATRIX_ALIGN_RIGHT
} MatrixText_Align_t;

typedef enum {
    MATRIX_SCROLL_LEFT = 0,
    MATRIX_SCROLL_RIGHT
} MatrixText_ScrollDir_t;

typedef struct {
    uint8_t fg_r;
    uint8_t fg_g;
    uint8_t fg_b;
    uint8_t bg_r;
    uint8_t bg_g;
    uint8_t bg_b;
    bool inverted;      // true: background is lit with fg color, text pixels use bg color
    bool mirror_x;      // true: horizontal mirror (e.g. rear-view mirror readable)
    bool mirror_y;      // true: vertical flip
    uint8_t spacing;    // pixels between characters (0 => defaults to 1)
} MatrixText_Style_t;

typedef struct {
    const char *text;
    MatrixText_Style_t style;
    MatrixText_ScrollDir_t dir;
    uint16_t step_ms;
    int16_t offset_x;
    int16_t text_width;
    uint32_t last_step_ms;
    bool loop_completed;
} MatrixText_Scroller_t;

/// @brief Return a default style (white text on black, 1px spacing).
MatrixText_Style_t MatrixText_DefaultStyle(uint8_t r, uint8_t g, uint8_t b);

/// @brief Measure total pixel width of a UTF-8 string (supports ASCII + Czech diacritics).
int16_t MatrixText_MeasureWidth(const char *utf8_text, uint8_t spacing);

/// @brief Draw a single Unicode codepoint at (x, y) using the 5x7 font.
void MatrixText_DrawCodepoint(int16_t x, int16_t y, uint32_t codepoint, const MatrixText_Style_t *style);

/// @brief Draw static UTF-8 text onto the WS2812B framebuffer (clears/fills background first).
void MatrixText_DrawStatic(const char *utf8_text,
                           int16_t x_offset,
                           MatrixText_Align_t align,
                           const MatrixText_Style_t *style);

/// @brief Initialize a non-blocking horizontal text scroller.
void MatrixText_ScrollInit(MatrixText_Scroller_t *scroller,
                           const char *utf8_text,
                           MatrixText_ScrollDir_t dir,
                           uint16_t step_ms,
                           const MatrixText_Style_t *style);

/// @brief Advance scroller if step_ms elapsed and push frame to WS2812B_Show().
/// @return true if a new frame was rendered and queued for DMA.
bool MatrixText_ScrollUpdate(MatrixText_Scroller_t *scroller, uint32_t now_ms);

/// @brief Run built-in text demo (Static, Inverted, Mirrored, Scroll Left/Right with Czech text).
void MatrixText_TestDemo_Run(uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif // MARALIB_MATRIXTEXT_H
