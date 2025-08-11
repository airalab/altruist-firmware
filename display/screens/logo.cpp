#ifdef ALTRUIST_INSIDE

#include "logo.h"
#include "../icons/icons/icons_200x200.h"
#include "../paint_driver/GUI_Paint.h"
#include "../driver/EPD.h"

void showLogoPage() {
    Paint_DrawImage(robo_hw_logo_black_200x200, DISPLAY_WIDTH / 2 - 100, DISPLAY_HEIGHT / 2 - 100, 200, 200);
}

#endif