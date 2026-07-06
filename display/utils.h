#ifdef ALTRUIST_INSIGHT

#ifndef _SCREENS_UTILS_H
#define _SCREENS_UTILS_H

#include "paint_driver/graphPainter.h"
#include "paint_driver/fonts/fonts.h"

void stringFromFloat(char *buffer, float value, int precision);
void stringFromFloat(char *buffer, float value);
void Paint_DrawString_EN_Center(const char * pString, sFONT* Font, UWORD Color_Foreground, UWORD Color_Background);

/* Draw string in current locale. RU: font_ru (Cyrillic) + font_ascii (ASCII). EN: font_ascii when available (same glyph style), else sFONT. Sizes 8/12/16/20/24 chosen per call via font_en/font_ru/font_ascii. */
void Paint_DrawString_Display(UWORD x, UWORD y, const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii, UWORD fg, UWORD bg);
/* Draw string for use on a black background (e.g. white text). In RU uses exact fg color (no flip). */
void Paint_DrawString_Display_OnBlack(UWORD x, UWORD y, const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii, UWORD fg, UWORD bg);
/* Draw with extra letter spacing (e.g. for bold/warning values). Width with spacing = GetStringWidth + (strlen - 1) * letter_spacing. */
void Paint_DrawString_Display_WithSpacing(UWORD x, UWORD y, const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii, UWORD fg, UWORD bg, int8_t letter_spacing);
uint16_t Paint_GetStringWidth_Display(const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii);
void Paint_DrawString_Display_Center(const char* str, sFONT* font_en, const Font* font_ru, const Font* font_ascii, UWORD fg, UWORD bg);

#endif
#endif