#ifdef ALTRUIST_INSIGHT

#include "utils.h"
#include "paint_driver/GUI_Paint.h"
#include "driver/EPD.h"
#include "../intl.h"
#include <stdlib.h>
#include <cstring>
#include <cstdio>


void stringFromFloat(char *buffer, float value, int precision) {
    // Clamp precision to valid range to avoid buffer overflows
    if (precision < 0) precision = 0;
    if (precision > 6) precision = 6;

    // Build format string dynamically, e.g., "%.3f"
    char format[8];
    snprintf(format, sizeof(format), "%%.%df", precision);

    // Format the float into the buffer
    snprintf(buffer, 32, format, value);

    // Remove trailing zeros and optional decimal point
    char *dot = strchr(buffer, '.');
    if (dot) {
        char *end = buffer + strlen(buffer) - 1;
        while (end > dot && *end == '0') {
            *end-- = '\0';
        }
        if (*end == '.') {
            *end = '\0'; // Remove decimal point if nothing follows
        }
    }
}

void stringFromFloat(char *buffer, float value) {
    stringFromFloat(buffer, value, 2);
}

void Paint_DrawString_EN_Center(const char * pString, sFONT* Font, UWORD Color_Foreground, UWORD Color_Background) {
    uint16_t x = DISPLAY_WIDTH / 2 - strlen(pString)*Font->Width / 2;
    uint16_t y = DISPLAY_HEIGHT / 2 - Font->Height / 2;
    Paint_DrawString_EN(x, y, pString, Font, Color_Foreground, Color_Background);
}

void Paint_DrawString_Display(UWORD x, UWORD y, const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii, UWORD fg, UWORD bg) {
#ifdef INTL_RU
    Paint_DrawString_RU(x, y, str, font_ru, fg, bg, font_en, font_ascii);
#else
    /* EN: use glyph-based font_ascii (same style as RU) when available; fallback to sFONT */
    if (font_ascii && font_ascii->count > 0) {
        Paint_DrawString_RU(x, y, str, font_ascii, fg, bg, font_en, font_ascii);
    } else {
        (void)font_ru;
        Paint_DrawString_EN(x, y, str, font_en, fg, bg);
    }
#endif
}

void Paint_DrawString_Display_OnBlack(UWORD x, UWORD y, const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii, UWORD fg, UWORD bg) {
#ifdef INTL_RU
    Paint_DrawString_RU_Ex(x, y, str, font_ru, fg, bg, font_en, font_ascii, true);
#else
    if (font_ascii && font_ascii->count > 0) {
        Paint_DrawString_RU_Ex(x, y, str, font_ascii, fg, bg, font_en, font_ascii, true);
    } else {
        (void)font_ru;
        Paint_DrawString_EN(x, y, str, font_en, fg, bg);
    }
#endif
}

void Paint_DrawString_Display_WithSpacing(UWORD x, UWORD y, const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii, UWORD fg, UWORD bg, int8_t letter_spacing) {
#ifdef INTL_RU
    Paint_DrawString_RU_WithSpacing(x, y, str, font_ru, fg, bg, font_en, font_ascii, letter_spacing);
#else
    if (font_ascii && font_ascii->count > 0) {
        Paint_DrawString_RU_WithSpacing(x, y, str, font_ascii, fg, bg, font_en, font_ascii, letter_spacing);
    } else {
        (void)font_ru;
        (void)letter_spacing;
        Paint_DrawString_EN(x, y, str, font_en, fg, bg);
    }
#endif
}

uint16_t Paint_GetStringWidth_Display(const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii) {
#ifdef INTL_RU
    return Paint_GetStringWidth_RU(str, font_ru, font_en, font_ascii);
#else
    if (font_ascii && font_ascii->count > 0) {
        return Paint_GetStringWidth_RU(str, font_ascii, font_en, font_ascii);
    }
    (void)font_ru;
    return (uint16_t)(strlen(str) * font_en->Width);
#endif
}

void Paint_DrawString_Display_Center(const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii, UWORD fg, UWORD bg) {
    uint16_t w = Paint_GetStringWidth_Display(str, font_en, font_ru, font_ascii);
    uint16_t x = (DISPLAY_WIDTH > w) ? (DISPLAY_WIDTH - w) / 2 : 0;
    uint16_t y = (DISPLAY_HEIGHT > font_en->Height) ? (DISPLAY_HEIGHT - font_en->Height) / 2 : 0;
#ifdef INTL_RU
    uint16_t line_height = font_ru->count ? font_ru->line_height : font_en->Height;
    if (DISPLAY_HEIGHT > line_height)
        y = (DISPLAY_HEIGHT - line_height) / 2;
#endif
    Paint_DrawString_Display(x, y, str, font_en, font_ru, font_ascii, fg, bg);
}

#endif