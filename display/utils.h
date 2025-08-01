#ifdef ALTRUIST_INSIDE

#ifndef _SCREENS_UTILS_H
#define _SCREENS_UTILS_H

#include "paint_driver/graphPainter.h"

void stringFromFloat(char *buffer, float value, int precision);
void stringFromFloat(char *buffer, float value);
void Paint_DrawString_EN_Center(const char * pString, sFONT* Font, UWORD Color_Foreground, UWORD Color_Background);

#endif
#endif