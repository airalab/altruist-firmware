#ifdef ALTRUIST_INSIDE

#ifndef DISPLAY_COMMON_H
#define DISPLAY_COMMON_H

#include "../driver/EPD.h"

void initAndClearScreen();
void createNewImage(UBYTE *&BlackImage);
void showImageFast(UBYTE *&BlackImage);
void showImageLong(UBYTE *&BlackImage);

#endif
#endif