#ifdef ALTRUIST_INSIGHT

#include "connecting.h"
#include "../utils.h"
#include "../paint_driver/GUI_Paint.h"
#include "../../config_manager/config_helpers.h"
#include "../../intl.h"
#include "../icons/icons/40x40/robo_hw_logo_black_40x40.h"  // team logo
#include "../icons/icons/35x35/wifi_35x35.h"  // wifi icon

// step goes from 0..100 (progress percentage)
void showConnectingPage(UBYTE *BlackImage, int step) {
    // Clear screen
    Paint_Clear(WHITE);

    // Create a more modern layout with better spacing
    int content_start_y = 30;

    // --- Header with logo and title together ---
    // Note: logo is actually 40x32, not 40x40 (see header file comment)
    int logo_x = (DISPLAY_WIDTH - 40) / 2;
    int logo_y = content_start_y;
    Paint_DrawImage(robo_hw_logo_black_40x40, logo_x, logo_y, 40, 32);

    // Title directly below logo with better spacing
    const char *title = INTL_DISP_TITLE_INSIGHT;
    int title_width = (int)Paint_GetStringWidth_Display(title, &Font20, &font_20_cyrillic, &font_20_ascii);
    int title_x = (DISPLAY_WIDTH - title_width) / 2;
    int title_y = logo_y + 50;
    Paint_DrawString_Display(title_x, title_y, title, &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);

    // --- Connection status section ---
    int status_section_y = title_y + Font20.Height + 25;
    
    // Connection status with Wi-Fi icon
    const char *status = INTL_DISP_CONNECTING_WIFI;
    int status_width = (int)Paint_GetStringWidth_Display(status, &Font16, &font_16_cyrillic, &font_16_ascii);
    int wifi_icon_size = 35; // Good size for 35x35 source
    int total_width = status_width + wifi_icon_size + 8; // 8px spacing
    int status_x = (DISPLAY_WIDTH - total_width) / 2;
    
    // Draw Wi-Fi icon (scaled from 35x35 to 20x20 - better scaling ratio)
    Paint_DrawImage(wifi_35x35, status_x, status_section_y, wifi_icon_size, wifi_icon_size);
    
    // Draw status text next to icon (same left edge for alignment)
    int status_text_x = status_x + wifi_icon_size + 8;
    Paint_DrawString_Display(status_text_x, status_section_y + 2, status, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

    // Network name: align with status text above, below the icon+text row with small gap
    char network_display[64];
    snprintf(network_display, sizeof(network_display), "\"%s\"", cfg::wlanssid);
    int status_row_height = (Font16.Height > wifi_icon_size) ? Font16.Height : wifi_icon_size;
    int network_x = status_text_x;
    const int gap_status_to_network = 0;
    int network_y = status_section_y + status_row_height + gap_status_to_network;
    Paint_DrawString_EN(network_x, network_y, network_display, &Font16, WHITE, BLACK);

    // --- Simple static dots ---
    int dots_y = network_y + Font16.Height + 20;
    int dot_spacing = 12;
    int num_dots = 5;
    int total_dots_width = num_dots * 6 + (num_dots - 1) * dot_spacing;
    int dots_start_x = (DISPLAY_WIDTH - total_dots_width) / 2;
    
    for (int i = 0; i < num_dots; i++) {
        int dot_x = dots_start_x + 3 + i * (6 + dot_spacing);
        Paint_DrawCircle(dot_x, dots_y, 3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }

    // --- Help text ---
    const char* help_text = INTL_DISP_PLEASE_WAIT;
    int help_width = (int)Paint_GetStringWidth_Display(help_text, &Font12, &font_12_cyrillic, &font_12_ascii);
    int help_x = (DISPLAY_WIDTH - help_width) / 2;
    int help_y = dots_y + 25;
    Paint_DrawString_Display(help_x, help_y, help_text, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
}

#endif
