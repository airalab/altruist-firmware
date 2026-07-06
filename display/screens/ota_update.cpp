#ifdef ALTRUIST_INSIGHT

#include "ota_update.h"
#include "../paint_driver/GUI_Paint.h"
#include "../utils.h"
#include "../../utils.h"
#include "../driver/EPD.h"
#include "../../intl.h"
#include "../paint_driver/fonts/fonts.h"
#include "../icons/icons/40x40/robo_hw_logo_black_40x40.h"
#include <stdio.h>

void showOTAUpdatePage(UBYTE *BlackImage, const device_status_t &deviceStatus) {
    Paint_Clear(WHITE);

    // Robonomics logo centered at top
    int logo_x = (DISPLAY_WIDTH - 40) / 2;
    int logo_y = 20;
    Paint_DrawImage(robo_hw_logo_black_40x40, logo_x, logo_y, 40, 32);

    // Main message: "Updating firmware"
    const char* line1 = INTL_DISP_OTA_UPDATING;
    uint16_t line1_w = Paint_GetStringWidth_Display(line1, &Font24, &font_24_cyrillic, &font_24_ascii);
    uint16_t line1_x = (DISPLAY_WIDTH > line1_w) ? (DISPLAY_WIDTH - line1_w) / 2 : 0;
    uint16_t line1_y = logo_y + 32 + 16;
    Paint_DrawString_Display(line1_x, line1_y, line1, &Font24, &font_24_cyrillic, &font_24_ascii, WHITE, BLACK);

    // Progress: "45%" when available
    uint16_t line2_y = line1_y + Font24.Height + 12;
    if (deviceStatus.ota_progress_percent >= 0) {
        char percent_buf[16];
        snprintf(percent_buf, sizeof(percent_buf), "%d%%", deviceStatus.ota_progress_percent);
        uint16_t pct_w = Paint_GetStringWidth_Display(percent_buf, &Font24, &font_24_cyrillic, &font_24_ascii);
        uint16_t pct_x = (DISPLAY_WIDTH > pct_w) ? (DISPLAY_WIDTH - pct_w) / 2 : 0;
        Paint_DrawString_Display(pct_x, line2_y, percent_buf, &Font24, &font_24_cyrillic, &font_24_ascii, WHITE, BLACK);
    }
    uint16_t line3_y = line2_y + Font24.Height + 8;

    // Warning: "Do not disconnect power"
    const char* line2 = INTL_DISP_OTA_DO_NOT_DISCONNECT;
    uint16_t line2_w = Paint_GetStringWidth_Display(line2, &Font16, &font_16_cyrillic, &font_16_ascii);
    uint16_t line2_x = (DISPLAY_WIDTH > line2_w) ? (DISPLAY_WIDTH - line2_w) / 2 : 0;
    Paint_DrawString_Display(line2_x, line3_y, line2, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

    // Sub message: "Please wait..."
    const char* line3 = INTL_DISP_PLEASE_WAIT;
    uint16_t line3_w = Paint_GetStringWidth_Display(line3, &Font16, &font_16_cyrillic, &font_16_ascii);
    uint16_t line3_x = (DISPLAY_WIDTH > line3_w) ? (DISPLAY_WIDTH - line3_w) / 2 : 0;
    uint16_t line4_y = line3_y + Font16.Height + 8;
    Paint_DrawString_Display(line3_x, line4_y, line3, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
}

void showOTAFailedPage(UBYTE *BlackImage) {
    Paint_Clear(WHITE);

    // Robonomics logo centered at top
    int logo_x = (DISPLAY_WIDTH - 40) / 2;
    int logo_y = 30;
    Paint_DrawImage(robo_hw_logo_black_40x40, logo_x, logo_y, 40, 32);

    // Main message: "Update failed"
    const char* line1 = INTL_DISP_OTA_FAILED;
    uint16_t line1_w = Paint_GetStringWidth_Display(line1, &Font24, &font_24_cyrillic, &font_24_ascii);
    uint16_t line1_x = (DISPLAY_WIDTH > line1_w) ? (DISPLAY_WIDTH - line1_w) / 2 : 0;
    uint16_t line1_y = logo_y + 32 + 20;
    Paint_DrawString_Display(line1_x, line1_y, line1, &Font24, &font_24_cyrillic, &font_24_ascii, WHITE, BLACK);

    // Sub message: "Will retry later"
    const char* line2 = INTL_DISP_OTA_WILL_RETRY;
    uint16_t line2_w = Paint_GetStringWidth_Display(line2, &Font16, &font_16_cyrillic, &font_16_ascii);
    uint16_t line2_x = (DISPLAY_WIDTH > line2_w) ? (DISPLAY_WIDTH - line2_w) / 2 : 0;
    uint16_t line2_y = line1_y + Font24.Height + 12;
    Paint_DrawString_Display(line2_x, line2_y, line2, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
}

void showOTASuccessPage(UBYTE *BlackImage) {
    Paint_Clear(WHITE);

    int logo_x = (DISPLAY_WIDTH - 40) / 2;
    int logo_y = 30;
    Paint_DrawImage(robo_hw_logo_black_40x40, logo_x, logo_y, 40, 32);

    const char* line1 = INTL_DISP_OTA_SUCCESS;
    uint16_t line1_w = Paint_GetStringWidth_Display(line1, &Font24, &font_24_cyrillic, &font_24_ascii);
    uint16_t line1_x = (DISPLAY_WIDTH > line1_w) ? (DISPLAY_WIDTH - line1_w) / 2 : 0;
    uint16_t line1_y = logo_y + 32 + 20;
    Paint_DrawString_Display(line1_x, line1_y, line1, &Font24, &font_24_cyrillic, &font_24_ascii, WHITE, BLACK);

    const char* line2 = INTL_DISP_OTA_RESTARTING;
    uint16_t line2_w = Paint_GetStringWidth_Display(line2, &Font16, &font_16_cyrillic, &font_16_ascii);
    uint16_t line2_x = (DISPLAY_WIDTH > line2_w) ? (DISPLAY_WIDTH - line2_w) / 2 : 0;
    uint16_t line2_y = line1_y + Font24.Height + 12;
    Paint_DrawString_Display(line2_x, line2_y, line2, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
}

#endif
