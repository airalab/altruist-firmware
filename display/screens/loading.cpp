#ifdef ALTRUIST_INSIDE

#include "loading.h"
#include "../../config_manager/config_helpers.h"
#include "main_screen.h"
#include "../driver/EPD.h"
#include "loading.h"
#include "display_common.h"
#include "../../utils.h"

void showLoadingPage(UBYTE *BlackImage) {
    Paint_DrawString_EN(DISPLAY_WIDTH / 2 - 5*Font24.Width, DISPLAY_HEIGHT / 2 - Font24.Height / 2, "Loading...", &Font24, WHITE, BLACK);
}

#endif