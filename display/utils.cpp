#ifdef ALTRUIST_INSIDE

#include "utils.h"
#include "paint_driver/GUI_Paint.h"
#include "driver/EPD.h"
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

#endif