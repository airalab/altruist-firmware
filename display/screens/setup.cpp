#ifdef ALTRUIST_INSIGHT

#include <qrcode.h>
#include "../../config_manager/config_helpers.h"
#include "main_screen.h"
#include "../driver/EPD.h"
#include "setup.h"
#include "../utils.h"
#include "display_common.h"
#include "../../intl.h"
#include "../paint_driver/fonts/fonts.h"
#include "../icons/icons/40x40/wifi_40x40.h"
#include "../icons/icons/40x40/robo_hw_logo_black_40x40.h"

QRCode qrcode;

void showSetupPage(UBYTE *BlackImage) {
    // Clear screen
    Paint_Clear(WHITE);

    // Draw team logo top-left
    // Note: logo is actually 40x32, not 40x40 (see header file comment)
    Paint_DrawImage(robo_hw_logo_black_40x40, 5, 5, 40, 32);

    // Draw Wi-Fi icon top-right
    Paint_DrawImage(wifi_40x40, DISPLAY_WIDTH - 45, 5, 40, 40);

    // Title centered
    const char* title = INTL_DISP_WIFI_SETUP;
    int title_x = (DISPLAY_WIDTH - (int)Paint_GetStringWidth_Display(title, &Font24, &font_24_cyrillic, &font_24_ascii)) / 2;
    int title_y = 10;
    Paint_DrawString_Display(title_x, title_y, title, &Font24, &font_24_cyrillic, &font_24_ascii, BLACK, WHITE);

    // QR code generation
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    char qr_data[100];
    sprintf(qr_data, "WIFI:S:%s;T:WPA2;P:%s;;", cfg::fs_ssid, cfg::fs_pwd);
    qrcode_initText(&qrcode, qrcodeData, QR_VERSION, ECC_LOW, qr_data);

    int scale_factor = 4; // pixels per module
    int quiet_zone = 3;
    int total_width = qrcode.size * scale_factor + 2 * quiet_zone;
    int total_height = qrcode.size * scale_factor + 2 * quiet_zone;
    int qr_bitmap_width_bytes = (total_width + 7) / 8;
    int qr_bitmap_size = total_height * qr_bitmap_width_bytes;

    unsigned char *qr_bitmap_scaled = (unsigned char*)malloc(qr_bitmap_size);
    memset(qr_bitmap_scaled, 0x00, qr_bitmap_size); // White background

    // Draw QR code into bitmap
    for (uint8_t qr_y = 0; qr_y < qrcode.size; qr_y++) {
        for (uint8_t qr_x = 0; qr_x < qrcode.size; qr_x++) {
            if (qrcode_getModule(&qrcode, qr_x, qr_y)) {
                for (int sy = 0; sy < scale_factor; sy++) {
                    for (int sx = 0; sx < scale_factor; sx++) {
                        int pixel_x = quiet_zone + qr_x * scale_factor + sx;
                        int pixel_y = quiet_zone + qr_y * scale_factor + sy;
                        if (pixel_x >= 0 && pixel_x < total_width &&
                            pixel_y >= 0 && pixel_y < total_height) {
                            qr_bitmap_scaled[pixel_y * qr_bitmap_width_bytes + (pixel_x / 8)] |= (0x80 >> (pixel_x % 8));
                        }
                    }
                }
            }
        }
    }

    // Center QR code vertically
    int qr_x = DISPLAY_WIDTH / 2 - total_width / 2;
    int qr_y = DISPLAY_HEIGHT / 2 - total_height / 2;
    Paint_DrawImage(qr_bitmap_scaled, qr_x, qr_y, total_width, total_height);

    int label_y = qr_y + total_height + (DISPLAY_HEIGHT - (qr_y + total_height)) / 2 - (3 * Font16.Height + 10) / 2;

    const char* connect_label = INTL_DISP_CONNECT_TO;
    int connect_w = (int)Paint_GetStringWidth_Display(connect_label, &Font16, &font_16_cyrillic, &font_16_ascii);
    Paint_DrawString_Display(DISPLAY_WIDTH / 2 - connect_w / 2, label_y, connect_label, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    int ssid_w = (int)Paint_GetStringWidth_Display(cfg::fs_ssid, &Font16, &font_16_cyrillic, &font_16_ascii);
    Paint_DrawString_Display(DISPLAY_WIDTH / 2 - ssid_w / 2, label_y + Font16.Height + 5, cfg::fs_ssid, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    char output_str[100];
    strcpy(output_str, INTL_DISP_PASSWORD_PREFIX);
    strcat(output_str, cfg::fs_pwd);
    int pwd_w = (int)Paint_GetStringWidth_Display(output_str, &Font16, &font_16_cyrillic, &font_16_ascii);
    Paint_DrawString_Display(DISPLAY_WIDTH / 2 - pwd_w / 2, label_y + 2*Font16.Height + 10, output_str, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    
    // Clean up
    free(qr_bitmap_scaled);
}

#endif
