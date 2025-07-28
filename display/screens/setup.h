#ifdef ALTRUIST_INSIDE

#ifndef _SETUP_SCREEN_H
#define _SETUP_SCREEN_H

#include "../driver/EPD.h"

#define QR_VERSION 3
#define QR_SIZE 29  // Version 3 = 29x29
#define BITMAP_WIDTH 32  // Must be multiple of 8 for byte alignment
#define BITMAP_HEIGHT 32
#define QR_SCALE 3

void showSetupPage(UBYTE *BlackImage);

#endif // _SETUP_SCREEN_H

#endif