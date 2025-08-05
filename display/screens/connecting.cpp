#ifdef ALTRUIST_INSIDE

#include "connecting.h"
#include "../paint_driver/GUI_Paint.h"
#include "../utils.h"
#include "../../config_manager/config_helpers.h"

void showConnectingPage(UBYTE *BlackImage) {
    Paint_DrawString_EN(DISPLAY_WIDTH / 2 - 15*Font24.Width / 2, DISPLAY_HEIGHT / 2 - Font24.Height - 5, "Connecting to", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(DISPLAY_WIDTH / 2 - strlen(cfg::wlanssid)*Font24.Width / 2, DISPLAY_HEIGHT / 2 + 5, cfg::wlanssid, &Font24, WHITE, BLACK);
}

#endif