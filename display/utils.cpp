#ifdef ALTRUIST_INSIDE

#include "utils.h"
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

#endif