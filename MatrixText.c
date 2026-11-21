#include "MatrixText.h"
#include "WS2812B.h"
#include <stddef.h>

#define FONT_GLYPH_WIDTH  5U
#define FONT_GLYPH_HEIGHT 7U

// Standard 5x7 ASCII font (0x20..0x7E), 5 columns per char, LSB = top row (y=0)
static const uint8_t s_font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '\''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // '}'
    {0x08, 0x04, 0x08, 0x10, 0x08}  // '~'
};

typedef struct {
    uint32_t codepoint;
    uint8_t cols[5];
} CzechGlyph_t;

// Compact 5x7 Czech glyphs (diacritics occupy rows 0..1, body occupies rows 2..6)
static const CzechGlyph_t s_czech_glyphs[] = {
    // Uppercase Czech
    {0x00C1, {0x78, 0x14, 0x16, 0x15, 0x78}}, // Á
    {0x010C, {0x38, 0x45, 0x46, 0x45, 0x24}}, // Č
    {0x010E, {0x7C, 0x45, 0x46, 0x25, 0x18}}, // Ď
    {0x00C9, {0x7C, 0x54, 0x56, 0x55, 0x44}}, // É
    {0x011A, {0x7C, 0x55, 0x56, 0x55, 0x44}}, // Ě
    {0x00CD, {0x00, 0x44, 0x7E, 0x45, 0x00}}, // Í
    {0x0147, {0x7C, 0x09, 0x12, 0x21, 0x7C}}, // Ň
    {0x00D3, {0x38, 0x44, 0x46, 0x45, 0x38}}, // Ó
    {0x0158, {0x7C, 0x15, 0x36, 0x55, 0x08}}, // Ř
    {0x0160, {0x48, 0x55, 0x56, 0x55, 0x24}}, // Š
    {0x0164, {0x04, 0x05, 0x7E, 0x05, 0x04}}, // Ť
    {0x00DA, {0x3C, 0x40, 0x42, 0x41, 0x3C}}, // Ú
    {0x016E, {0x3C, 0x42, 0x45, 0x42, 0x3C}}, // Ů
    {0x00DD, {0x0C, 0x10, 0x62, 0x11, 0x0C}}, // Ý
    {0x017D, {0x44, 0x65, 0x56, 0x4D, 0x44}}, // Ž

    // Lowercase Czech
    {0x00E1, {0x20, 0x54, 0x56, 0x55, 0x78}}, // á
    {0x010D, {0x38, 0x45, 0x46, 0x45, 0x20}}, // č
    {0x010F, {0x38, 0x44, 0x44, 0x48, 0x7D}}, // ď
    {0x00E9, {0x38, 0x54, 0x56, 0x55, 0x18}}, // é
    {0x011B, {0x38, 0x55, 0x56, 0x55, 0x18}}, // ě
    {0x00ED, {0x00, 0x44, 0x7C, 0x42, 0x01}}, // í
    {0x0148, {0x78, 0x09, 0x06, 0x05, 0x78}}, // ň
    {0x00F3, {0x38, 0x44, 0x46, 0x45, 0x38}}, // ó
    {0x0159, {0x78, 0x11, 0x0A, 0x09, 0x10}}, // ř
    {0x0161, {0x48, 0x55, 0x56, 0x55, 0x20}}, // š
    {0x0165, {0x08, 0x3E, 0x48, 0x41, 0x22}}, // ť
    {0x00FA, {0x38, 0x40, 0x42, 0x21, 0x78}}, // ú
    {0x016F, {0x38, 0x42, 0x45, 0x22, 0x78}}, // ů
    {0x00FD, {0x0C, 0x50, 0x52, 0x51, 0x3C}}, // ý
    {0x017E, {0x44, 0x65, 0x56, 0x4D, 0x44}}, // ž
};

static uint32_t utf8_next_codepoint(const char **p) {
    const uint8_t *s = (const uint8_t *)(*p);
    if (*s == 0U) {
        return 0U;
    }

    uint32_t cp;
    if ((s[0] & 0x80U) == 0U) {
        cp = s[0];
        *p += 1;
    } else if ((s[0] & 0xE0U) == 0xC0U && (s[1] & 0xC0U) == 0x80U) {
        cp = ((uint32_t)(s[0] & 0x1FU) << 6) | (uint32_t)(s[1] & 0x3FU);
        *p += 2;
    } else if ((s[0] & 0xF0U) == 0xE0U && (s[1] & 0xC0U) == 0x80U && (s[2] & 0xC0U) == 0x80U) {
        cp = ((uint32_t)(s[0] & 0x0FU) << 12) |
             ((uint32_t)(s[1] & 0x3FU) << 6) |
             (uint32_t)(s[2] & 0x3FU);
        *p += 3;
    } else {
        cp = '?';
        *p += 1;
    }
    return cp;
}

static const uint8_t *lookup_glyph(uint32_t codepoint) {
    if (codepoint >= 0x20U && codepoint <= 0x7EU) {
        return s_font5x7[codepoint - 0x20U];
    }
    const size_t count = sizeof(s_czech_glyphs) / sizeof(s_czech_glyphs[0]);
    for (size_t i = 0U; i < count; i++) {
        if (s_czech_glyphs[i].codepoint == codepoint) {
            return s_czech_glyphs[i].cols;
        }
    }
    return s_font5x7['?' - 0x20U];
}

static void put_pixel_transformed(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b, const MatrixText_Style_t *style) {
    if (x < 0 || x >= (int16_t)WS2812B_WIDTH || y < 0 || y >= (int16_t)WS2812B_HEIGHT) {
        return;
    }
    uint8_t tx = style->mirror_x ? (uint8_t)((WS2812B_WIDTH - 1U) - (uint8_t)x) : (uint8_t)x;
    uint8_t ty = style->mirror_y ? (uint8_t)((WS2812B_HEIGHT - 1U) - (uint8_t)y) : (uint8_t)y;
    WS2812B_SetPixelXY(tx, ty, r, g, b);
}

static void fill_background(const MatrixText_Style_t *style) {
    if (style->inverted) {
        WS2812B_Fill(style->fg_r, style->fg_g, style->fg_b);
    } else {
        WS2812B_Fill(style->bg_r, style->bg_g, style->bg_b);
    }
}

MatrixText_Style_t MatrixText_DefaultStyle(uint8_t r, uint8_t g, uint8_t b) {
    MatrixText_Style_t s = {
        .fg_r = r,
        .fg_g = g,
        .fg_b = b,
        .bg_r = 0U,
        .bg_g = 0U,
        .bg_b = 0U,
        .inverted = false,
        .mirror_x = false,
        .mirror_y = false,
        .spacing = 1U
    };
    return s;
}

int16_t MatrixText_MeasureWidth(const char *utf8_text, uint8_t spacing) {
    if (utf8_text == NULL || *utf8_text == '\0') {
        return 0;
    }
    uint8_t sp = (spacing == 0U) ? 1U : spacing;
    int16_t chars = 0;
    const char *p = utf8_text;
    while (utf8_next_codepoint(&p) != 0U) {
        chars++;
    }
    if (chars == 0) {
        return 0;
    }
    return (int16_t)(chars * (int16_t)FONT_GLYPH_WIDTH + (chars - 1) * (int16_t)sp);
}

void MatrixText_DrawCodepoint(int16_t x, int16_t y, uint32_t codepoint, const MatrixText_Style_t *style) {
    if (style == NULL) {
        return;
    }
    const uint8_t *cols = lookup_glyph(codepoint);
    uint8_t pr = style->inverted ? style->bg_r : style->fg_r;
    uint8_t pg = style->inverted ? style->bg_g : style->fg_g;
    uint8_t pb = style->inverted ? style->bg_b : style->fg_b;

    for (uint8_t col = 0U; col < FONT_GLYPH_WIDTH; col++) {
        uint8_t col_bits = cols[col];
        for (uint8_t row = 0U; row < FONT_GLYPH_HEIGHT; row++) {
            if ((col_bits & (1U << row)) != 0U) {
                put_pixel_transformed((int16_t)(x + col), (int16_t)(y + row), pr, pg, pb, style);
            }
        }
    }
}

static void render_text_at(const char *utf8_text, int16_t start_x, const MatrixText_Style_t *style) {
    fill_background(style);
    if (utf8_text == NULL) {
        return;
    }
    uint8_t sp = (style->spacing == 0U) ? 1U : style->spacing;
    int16_t cursor_x = start_x;
    const char *p = utf8_text;
    uint32_t cp;

    while ((cp = utf8_next_codepoint(&p)) != 0U) {
        if ((cursor_x + (int16_t)FONT_GLYPH_WIDTH) > 0 && cursor_x < (int16_t)WS2812B_WIDTH) {
            MatrixText_DrawCodepoint(cursor_x, 0, cp, style);
        }
        cursor_x += (int16_t)(FONT_GLYPH_WIDTH + sp);
    }
}

void MatrixText_DrawStatic(const char *utf8_text,
                           int16_t x_offset,
                           MatrixText_Align_t align,
                           const MatrixText_Style_t *style) {
    if (style == NULL) {
        return;
    }
    uint8_t sp = (style->spacing == 0U) ? 1U : style->spacing;
    int16_t w = MatrixText_MeasureWidth(utf8_text, sp);
    int16_t base_x = 0;

    switch (align) {
        case MATRIX_ALIGN_CENTER:
            base_x = (int16_t)(((int16_t)WS2812B_WIDTH - w) / 2);
            break;
        case MATRIX_ALIGN_RIGHT:
            base_x = (int16_t)((int16_t)WS2812B_WIDTH - w);
            break;
        case MATRIX_ALIGN_LEFT:
        default:
            base_x = 0;
            break;
    }

    render_text_at(utf8_text, (int16_t)(base_x + x_offset), style);
}

void MatrixText_ScrollInit(MatrixText_Scroller_t *scroller,
                           const char *utf8_text,
                           MatrixText_ScrollDir_t dir,
                           uint16_t step_ms,
                           const MatrixText_Style_t *style) {
    if (scroller == NULL || style == NULL) {
        return;
    }
    scroller->text = utf8_text;
    scroller->style = *style;
    if (scroller->style.spacing == 0U) {
        scroller->style.spacing = 1U;
    }
    scroller->dir = dir;
    scroller->step_ms = (step_ms == 0U) ? 50U : step_ms;
    scroller->text_width = MatrixText_MeasureWidth(utf8_text, scroller->style.spacing);
    scroller->offset_x = (dir == MATRIX_SCROLL_LEFT)
                             ? (int16_t)WS2812B_WIDTH
                             : (int16_t)(-scroller->text_width);
    scroller->last_step_ms = 0U;
    scroller->loop_completed = false;
}

bool MatrixText_ScrollUpdate(MatrixText_Scroller_t *scroller, uint32_t now_ms) {
    if (scroller == NULL || WS2812B_IsBusy()) {
        return false;
    }
    if ((now_ms - scroller->last_step_ms) < scroller->step_ms) {
        return false;
    }
    scroller->last_step_ms = now_ms;

    render_text_at(scroller->text, scroller->offset_x, &scroller->style);
    WS2812B_Show();

    if (scroller->dir == MATRIX_SCROLL_LEFT) {
        scroller->offset_x--;
        if (scroller->offset_x < -scroller->text_width) {
            scroller->offset_x = (int16_t)WS2812B_WIDTH;
            scroller->loop_completed = true;
        }
    } else {
        scroller->offset_x++;
        if (scroller->offset_x > (int16_t)WS2812B_WIDTH) {
            scroller->offset_x = (int16_t)(-scroller->text_width);
            scroller->loop_completed = true;
        }
    }
    return true;
}

void MatrixText_TestDemo_Run(uint32_t now_ms) {
    typedef enum {
        DEMO_STATIC = 0,
        DEMO_INVERTED,
        DEMO_MIRRORED,
        DEMO_SCROLL_LEFT_CZ,
        DEMO_SCROLL_RIGHT_CZ
    } DemoStage_t;

    static DemoStage_t stage = DEMO_STATIC;
    static uint32_t stage_start_ms = 0U;
    static uint32_t last_static_draw_ms = 0U;
    static bool scroller_init = false;
    static MatrixText_Scroller_t scroller;

    if (WS2812B_IsBusy()) {
        return;
    }
    if (stage_start_ms == 0U) {
        stage_start_ms = now_ms;
    }

    switch (stage) {
        case DEMO_STATIC: {
            if ((now_ms - stage_start_ms) >= 2500U) {
                stage = DEMO_INVERTED;
                stage_start_ms = now_ms;
                last_static_draw_ms = 0U;
                return;
            }
            if ((now_ms - last_static_draw_ms) >= 100U) {
                last_static_draw_ms = now_ms;
                MatrixText_Style_t s = MatrixText_DefaultStyle(255U, 0U, 0U);
                MatrixText_DrawStatic("STOP", 0, MATRIX_ALIGN_CENTER, &s);
                WS2812B_Show();
            }
            break;
        }

        case DEMO_INVERTED: {
            if ((now_ms - stage_start_ms) >= 2500U) {
                stage = DEMO_MIRRORED;
                stage_start_ms = now_ms;
                last_static_draw_ms = 0U;
                return;
            }
            if ((now_ms - last_static_draw_ms) >= 100U) {
                last_static_draw_ms = now_ms;
                MatrixText_Style_t s = MatrixText_DefaultStyle(255U, 0U, 0U);
                s.inverted = true;
                MatrixText_DrawStatic("STOP", 0, MATRIX_ALIGN_CENTER, &s);
                WS2812B_Show();
            }
            break;
        }

        case DEMO_MIRRORED: {
            if ((now_ms - stage_start_ms) >= 2500U) {
                stage = DEMO_SCROLL_LEFT_CZ;
                stage_start_ms = now_ms;
                scroller_init = false;
                return;
            }
            if ((now_ms - last_static_draw_ms) >= 100U) {
                last_static_draw_ms = now_ms;
                MatrixText_Style_t s = MatrixText_DefaultStyle(0U, 60U, 255U);
                s.mirror_x = true;
                MatrixText_DrawStatic("STOP", 0, MATRIX_ALIGN_CENTER, &s);
                WS2812B_Show();
            }
            break;
        }

        case DEMO_SCROLL_LEFT_CZ: {
            if (!scroller_init) {
                MatrixText_Style_t s = MatrixText_DefaultStyle(0U, 100U, 255U);
                MatrixText_ScrollInit(&scroller, "POLICIE \xC4\x8CR - ZASTAVTE!", MATRIX_SCROLL_LEFT, 45U, &s);
                scroller_init = true;
            }
            MatrixText_ScrollUpdate(&scroller, now_ms);
            if (scroller.loop_completed) {
                stage = DEMO_SCROLL_RIGHT_CZ;
                stage_start_ms = now_ms;
                scroller_init = false;
            }
            break;
        }

        case DEMO_SCROLL_RIGHT_CZ: {
            if (!scroller_init) {
                MatrixText_Style_t s = MatrixText_DefaultStyle(255U, 110U, 0U);
                MatrixText_ScrollInit(&scroller, "POZOR NEHODA - OBJ\xC3\x8D\xC5\xBD\xC4\x8EKA", MATRIX_SCROLL_RIGHT, 45U, &s);
                scroller_init = true;
            }
            MatrixText_ScrollUpdate(&scroller, now_ms);
            if (scroller.loop_completed) {
                stage = DEMO_STATIC;
                stage_start_ms = now_ms;
                last_static_draw_ms = 0U;
                scroller_init = false;
            }
            break;
        }
    }
}
