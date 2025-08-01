#ifdef ALTRUIST_INSIDE

#include "loading.h"
#include "../paint_driver/GUI_Paint.h"
#include "../utils.h"

void showLoadingPage(UBYTE *BlackImage) {
    Paint_DrawString_EN_Center("Loading...", &Font24, WHITE, BLACK);
}

#endif