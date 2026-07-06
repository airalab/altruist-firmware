#ifdef ALTRUIST_INSIGHT

#include <qrcode.h>
#include <string.h>
#include "../../config_manager/config_helpers.h"
#include "main_screen.h"
#include "../driver/EPD.h"
#include "setup.h"
#include "../utils.h"
#include "../../utils.h"
#include "display_common.h"
#include "../../intl.h"
#include "../paint_driver/fonts/fonts.h"
#include "../paint_driver/GUI_Paint.h"
#include "../icons/icons/icons_15x15.h"

QRCode QRSensorMap;

static void drawCenteredFittedText(const char* text, int y, int x_left, int x_right, sFONT* font,
                                   const Font* font_ru_override = nullptr, const Font* font_ascii_override = nullptr) {
    if (text == nullptr || font == nullptr || x_right <= x_left) return;

    const int max_width = x_right - x_left + 1;
    const size_t src_len = strlen(text);
    const size_t cap = 159;
    char fitted[160];

    size_t n = (src_len < cap) ? src_len : cap;
    memcpy(fitted, text, n);
    fitted[n] = '\0';

    const Font* cyr = &font_12_cyrillic;
    const Font* ascii = &font_12_ascii;
    if (font == &Font16) {
        cyr = &font_16_cyrillic;
        ascii = &font_16_ascii;
    } else if (font == &Font20) {
        cyr = &font_20_cyrillic;
        ascii = &font_20_ascii;
    } else if (font == &Font24) {
        cyr = &font_24_cyrillic;
        ascii = &font_24_ascii;
    }
    if (font_ru_override != nullptr) cyr = font_ru_override;
    if (font_ascii_override != nullptr) ascii = font_ascii_override;

    const bool may_need_ellipsis = (src_len > n);
    while (n > 0) {
        int w = (int)Paint_GetStringWidth_Display(fitted, font, cyr, ascii);
        if (w <= max_width) break;
        n--;
        fitted[n] = '\0';
    }
    if (n == 0) return;

    // If we had to truncate, add ".." so the cut isn't confusing (e.g. "smartpho").
    if (src_len > n && n > 2) {
        // Prefer trimming trailing spaces before adding dots
        while (n > 2 && fitted[n - 1] == ' ') {
            fitted[n - 1] = '\0';
            n--;
        }
        fitted[n - 1] = '.';
        fitted[n - 2] = '.';
        // Ensure still fits; if not, shrink further.
        while (n > 2) {
            int w = (int)Paint_GetStringWidth_Display(fitted, font, cyr, ascii);
            if (w <= max_width) break;
            n--;
            fitted[n] = '\0';
            if (n > 2) {
                fitted[n - 1] = '.';
                fitted[n - 2] = '.';
            }
        }
    }

    int w = (int)Paint_GetStringWidth_Display(fitted, font, cyr, ascii);
    int x = x_left + (max_width - w) / 2;
    if (x < x_left) x = x_left;
    Paint_DrawString_Display(x, y, fitted, font, cyr, ascii, WHITE, BLACK);
}

static bool bitmapGetBit(const unsigned char* buf, int width, int x, int y) {
    if (buf == nullptr || width <= 0 || x < 0 || y < 0 || x >= width) return false;
    int bytes_per_row = (width + 7) / 8;
    int byte_index = y * bytes_per_row + (x / 8);
    return (buf[byte_index] & (0x80 >> (x % 8))) != 0;
}

static void bitmapSetBit(unsigned char* buf, int width, int x, int y) {
    if (buf == nullptr || width <= 0 || x < 0 || y < 0 || x >= width) return;
    int bytes_per_row = (width + 7) / 8;
    int byte_index = y * bytes_per_row + (x / 8);
    buf[byte_index] |= (0x80 >> (x % 8));
}

void showSensorsMapPage(const String& robonomics_address) {
    uint8_t qrcodeData[qrcode_getBufferSize(12)];
    const char* sensor_ss58 = (robonomics_address.length() > 0) ? robonomics_address.c_str() : nullptr;
    const String map_url = buildSensorsSocialMapUrl(sensor_ss58);
    qrcode_initText(&QRSensorMap, qrcodeData, 12, ECC_LOW, map_url.c_str());

    int scale_factor = 1;
    const int quiet_zone = 3;
    int scaled_qr = QRSensorMap.size * scale_factor;
    int total_width = scaled_qr + (2 * quiet_zone);
    int total_height = total_width;

    // Keep QR guaranteed inside panel bounds.
    const int panel_max_qr = (DISPLAY_HEIGHT > 20) ? (DISPLAY_HEIGHT - 20) : DISPLAY_HEIGHT;
    if (total_height > panel_max_qr) {
        scale_factor = 1;
        scaled_qr = QRSensorMap.size * scale_factor;
        total_width = scaled_qr + (2 * quiet_zone);
        total_height = total_width;
    }

    int qr_bitmap_width_bytes = (total_width + 7) / 8;
    int qr_bitmap_size = total_height * qr_bitmap_width_bytes;
    unsigned char *qr_bitmap_scaled = (unsigned char*)malloc(qr_bitmap_size);
    if (!qr_bitmap_scaled) return;
    memset(qr_bitmap_scaled, 0x00, qr_bitmap_size);

    for (uint8_t qr_y = 0; qr_y < QRSensorMap.size; qr_y++) {
        for (uint8_t qr_x = 0; qr_x < QRSensorMap.size; qr_x++) {
            if (qrcode_getModule(&QRSensorMap, qr_x, qr_y)) {
                for (int sy = 0; sy < scale_factor; sy++) {
                    for (int sx = 0; sx < scale_factor; sx++) {
                        int px = quiet_zone + qr_x * scale_factor + sx;
                        int py = quiet_zone + qr_y * scale_factor + sy;
                        int bi = py * qr_bitmap_width_bytes + (px / 8);
                        qr_bitmap_scaled[bi] |= (0x80 >> (px % 8));
                    }
                }
            }
        }
    }


    int render_width = total_width;
    int render_height = total_height;
    unsigned char* qr_bitmap_render = qr_bitmap_scaled;
    unsigned char* qr_bitmap_scaled_15 = nullptr;
    const bool upscale_1_5 = true;
    if (upscale_1_5) {
        render_width = (total_width * 3) / 2;
        render_height = (total_height * 3) / 2;
        int render_bytes = (render_width + 7) / 8;
        int render_size = render_height * render_bytes;
        qr_bitmap_scaled_15 = (unsigned char*)malloc(render_size);
        if (qr_bitmap_scaled_15 != nullptr) {
            memset(qr_bitmap_scaled_15, 0x00, render_size);
            for (int y = 0; y < render_height; y++) {
                int src_y = (y * 2) / 3;
                for (int x = 0; x < render_width; x++) {
                    int src_x = (x * 2) / 3;
                    if (bitmapGetBit(qr_bitmap_scaled, total_width, src_x, src_y)) {
                        bitmapSetBit(qr_bitmap_scaled_15, render_width, x, y);
                    }
                }
            }
            qr_bitmap_render = qr_bitmap_scaled_15;
        } else {
            render_width = total_width;
            render_height = total_height;
        }
    }

    Paint_Clear(WHITE);

    const uint16_t nav_sidebar_width = 26;
    const uint16_t content_left = 0;
    const uint16_t content_right = (DISPLAY_WIDTH > nav_sidebar_width + 1) ? (DISPLAY_WIDTH - nav_sidebar_width - 1) : (DISPLAY_WIDTH - 1);
    const uint16_t content_width = (content_right > content_left) ? (content_right - content_left) : 0;

    // Top status bar.
    struct tm timeinfo;
    const uint16_t header_top_y = 6;
    const uint16_t header_row_height = Font16.Height + 2;
    const uint16_t header_bottom_y = header_top_y + header_row_height + 2;

    const uint16_t header_icon_x = 4;
    Paint_DrawImage(map_nav_15x15, header_icon_x, header_top_y, 15, 15);

    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M", &timeinfo);
        int tw = (int)Paint_GetStringWidth_Display(time_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        Paint_DrawString_Display((DISPLAY_WIDTH - tw) / 2, header_top_y, time_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        int dw = (int)Paint_GetStringWidth_Display(date_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - dw;
        Paint_DrawString_Display(date_x, header_top_y, date_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    }

    Paint_DrawLine(0, header_bottom_y, DISPLAY_WIDTH, header_bottom_y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Main promo content block.
    const uint16_t content_top_y = header_bottom_y + 12;
    const uint16_t content_bottom_y = DISPLAY_HEIGHT - 8;
    const int content_center_x = content_left + content_width / 2;

    const char* title = INTL_DISP_MAP_PROMO_TITLE;
    // Use 18px glyph font (less tall than 20, still prominent).
    drawCenteredFittedText(title, content_top_y, content_left + 6, content_right - 6, &Font20,
                           &font_18_cyrillic, &font_18_ascii);

    const char* subtitle_line_1 = INTL_DISP_MAP_PROMO_LINE1;
    const char* subtitle_line_2 = INTL_DISP_MAP_PROMO_LINE2;
    const char* subtitle_line_3 = INTL_DISP_MAP_PROMO_LINE3;
    const int subtitle_line_gap = 3;
    const int subtitle_line_height =
#ifdef INTL_RU
        (font_14_cyrillic.line_height ? (int)font_14_cyrillic.line_height : (int)Font16.Height);
#else
        (font_14_ascii.line_height ? (int)font_14_ascii.line_height : (int)Font16.Height);
#endif
    const int subtitle_top = content_top_y + Font20.Height + 14;
    drawCenteredFittedText(subtitle_line_1, subtitle_top, content_left, content_right, &Font16,
                           &font_14_cyrillic, &font_14_ascii);
    drawCenteredFittedText(subtitle_line_2, subtitle_top + (subtitle_line_height + subtitle_line_gap), content_left, content_right, &Font16,
                           &font_14_cyrillic, &font_14_ascii);
    drawCenteredFittedText(subtitle_line_3, subtitle_top + 2 * (subtitle_line_height + subtitle_line_gap), content_left, content_right, &Font16,
                           &font_14_cyrillic, &font_14_ascii);
    int subtitle_bottom = subtitle_top + 3 * (subtitle_line_height + subtitle_line_gap);

    int qr_x = content_left + ((int)content_width - render_width) / 2;
    if (qr_x < (int)content_left + 6) qr_x = content_left + 6;
    int qr_x_max = content_right - render_width - 2;
    if (qr_x > qr_x_max) qr_x = qr_x_max;
    if (qr_x < 0) qr_x = 0;
    const int qr_min_y = subtitle_bottom + 2;
    int qr_max_y = content_bottom_y - (Font16.Height + 12) - render_height;
    if (qr_max_y < qr_min_y) qr_max_y = qr_min_y;
    int qr_y = qr_min_y + (qr_max_y - qr_min_y) / 2;
    int qr_y_max_panel = DISPLAY_HEIGHT - render_height - 2;
    if (qr_y > qr_y_max_panel) qr_y = qr_y_max_panel;
    if (qr_y < 0) qr_y = 0;

    Paint_DrawImage(qr_bitmap_render, qr_x, qr_y, render_width, render_height);

    const char* domain = INTL_DISP_MAP_DOMAIN;
    int domain_y = qr_y + render_height + 12;
    const int domain_y_max = content_bottom_y - Font16.Height;
    if (domain_y > domain_y_max) domain_y = domain_y_max;
    drawCenteredFittedText(domain, domain_y, content_left + 6, content_right - 6, &Font16);

    if (qr_bitmap_scaled_15 != nullptr) {
        free(qr_bitmap_scaled_15);
    }
    free(qr_bitmap_scaled);
}

#endif
