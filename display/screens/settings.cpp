#ifdef ALTRUIST_INSIGHT

#include <qrcode.h>
#include <WiFi.h>
#include "../../config_manager/config_helpers.h"
#include "../driver/EPD.h"
#include "settings.h"
#include "../utils.h"
#include "../../utils.h"
#include "../../defines.h"
#include "../paint_driver/GUI_Paint.h"
#if defined(USE_SD_CARD)
#include "../../sd_card/sd_card.h"
#endif
#include "../icons/icons/40x40/robo_hw_logo_black_40x40.h"
#include "../icons/icons/15x15/ip_address_15x15.h"
#include "../icons/icons/15x15/wifi_15x15.h"
#include "../icons/icons/15x15/sd_card_15x15.h"
#include "../icons/icons/15x15/location_15x15.h"
#include "../icons/icons/15x15/circuit_15x15.h"
#include "../icons/icons/15x15/settings_15x15.h"
#include "../../intl.h"
#include "../paint_driver/fonts/fonts.h"

QRCode settingsQR;

// Helper function to draw text with word wrapping (no word cutting)
void drawWrappedText(int x, int y, const String &text, sFONT* font, UWORD color_fg, UWORD color_bg, int max_width, int &actual_height) {
    int current_x = x;
    int current_y = y;
    int line_height = font->Height;
    actual_height = line_height;
    
    String remaining = text;
    while (remaining.length() > 0) {
        // Calculate how many characters fit on this line
        int chars_per_line = max_width / font->Width;
        if (chars_per_line <= 0) chars_per_line = 1;
        
        String line;
        if (remaining.length() <= chars_per_line) {
            // Entire remaining text fits
            line = remaining;
            remaining = "";
        } else {
            // Find the last space before the limit to avoid cutting words
            int break_pos = chars_per_line;
            bool found_space = false;
            for (int i = chars_per_line; i > 0 && i > chars_per_line - 15; i--) {
                char c = remaining.charAt(i);
                if (c == ' ' || c == '.' || c == ':' || c == '-' || c == '/') {
                    break_pos = i + 1;
                    found_space = true;
                    break;
                }
            }
            // If no space found, allow breaking at the limit (but try to avoid mid-word)
            if (!found_space && chars_per_line > 10) {
                for (int i = chars_per_line; i > chars_per_line - 5; i--) {
                    char c = remaining.charAt(i);
                    if (c == '.' || c == '-' || c == '/') {
                        break_pos = i + 1;
                        found_space = true;
                        break;
                    }
                }
            }
            line = remaining.substring(0, break_pos);
            remaining = remaining.substring(break_pos);
            remaining.trim(); 
        }
        
        Paint_DrawString_EN(current_x, current_y, line.c_str(), font, color_fg, color_bg);
        current_y += line_height;
        actual_height += line_height;
    }
}

void showSettingsPage(UBYTE *BlackImage, device_status_t &deviceStatus, const String &urban_ip, const String &robonomics_address) {
    // Clear screen
    Paint_Clear(WHITE);

    // === HEADER: same layout as main screen (icon + time + date) ===
    struct tm timeinfo;
    const uint16_t header_top_y = 6;
    const uint16_t header_row_height = Font16.Height + 2;
    uint16_t header_bottom_border_y = header_top_y + header_row_height + 2;

    // Reserve space for navigation icons on right in the rest of the page
    const uint16_t nav_sidebar_width = 28;
    uint16_t usable_width = DISPLAY_WIDTH - nav_sidebar_width - 10;
    uint16_t content_left = 12;

    // Left: settings icon (page icon)
    const uint16_t header_icon_size = 15;
    const uint16_t header_icon_x    = 4;
    const uint16_t header_icon_y    = header_top_y;
    Paint_DrawImage(settings_15x15, header_icon_x, header_icon_y, header_icon_size, header_icon_size);

    // Center: time (same display font as rest of UI)
    if (getLocalTime(&timeinfo)) {
        char date_buf[12], time_buf[8];
        strftime(date_buf, sizeof(date_buf), "%m/%d/%Y", &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%H:%M",    &timeinfo);

        int time_width = (int)Paint_GetStringWidth_Display(time_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        int time_x = (DISPLAY_WIDTH - time_width) / 2;
        int time_y = header_top_y;
        Paint_DrawString_Display(time_x, time_y, time_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);

        // Right: date 
        int date_width = (int)Paint_GetStringWidth_Display(date_buf, &Font16, &font_16_cyrillic, &font_16_ascii);
        const int right_margin = 4;
        int date_x = DISPLAY_WIDTH - right_margin - date_width;
        int date_y = header_top_y;
        Paint_DrawString_Display(date_x, date_y, date_buf, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    }

    // Header bottom border
    Paint_DrawLine(0, header_bottom_border_y, DISPLAY_WIDTH, header_bottom_border_y,
                   BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    
    // Generate QR code first for header-like device info block under the top bar
    String qr_url = "http://" + deviceStatus.ip_address;
    if (deviceStatus.ip_address.length() == 0) {
        qr_url = "http://192.168.4.1"; // Fallback to IP
    }
    
    uint8_t qrcodeData[qrcode_getBufferSize(5)];
    char qr_data[100];
    snprintf(qr_data, sizeof(qr_data), "%s", qr_url.c_str());
    qrcode_initText(&settingsQR, qrcodeData, 5, ECC_LOW, qr_data);

    int scale_factor = 1; // Smaller QR code
    int quiet_zone = 4;
    int total_width = settingsQR.size * scale_factor + 2 * quiet_zone;
    int total_height = settingsQR.size * scale_factor + 2 * quiet_zone;
    int qr_bitmap_width_bytes = (total_width + 7) / 8;
    int qr_bitmap_size = total_height * qr_bitmap_width_bytes;

    unsigned char *qr_bitmap_scaled = (unsigned char*)malloc(qr_bitmap_size);
    if (qr_bitmap_scaled) {
        memset(qr_bitmap_scaled, 0x00, qr_bitmap_size); // White background

        // Draw QR code into bitmap
        for (uint8_t qr_y = 0; qr_y < settingsQR.size; qr_y++) {
            for (uint8_t qr_x = 0; qr_x < settingsQR.size; qr_x++) {
                if (qrcode_getModule(&settingsQR, qr_x, qr_y)) {
                    for (int sy = 0; sy < scale_factor; sy++) {
                        for (int sx = 0; sx < scale_factor; sx++) {
                            int pixel_x = quiet_zone + qr_x * scale_factor + sx;
                            int pixel_y = quiet_zone + qr_y * scale_factor + sy;
                            if (pixel_x >= 0 && pixel_x < total_width &&
                                pixel_y >= 0 && pixel_y < total_height) {
                                int byte_index = pixel_y * qr_bitmap_width_bytes + (pixel_x / 8);
                                qr_bitmap_scaled[byte_index] |= (0x80 >> (pixel_x % 8));
                            }
                        }
                    }
                }
            }
        }
        
        // Two column layout: QR on left, text on right, below the top bar
        int qr_x = content_left;
        int qr_y = header_bottom_border_y + 6;
        Paint_DrawImage(qr_bitmap_scaled, qr_x, qr_y, total_width, total_height);
        
        // Text column on the right
        int text_x = qr_x + total_width + 10; // Space between QR and text
        int text_y = qr_y;
        
        // "Device info" title
        const char* device_info_title = INTL_DISP_DEVICE_INFO;
        Paint_DrawString_Display(text_x, text_y, device_info_title, &Font20, &font_20_cyrillic, &font_20_ascii, WHITE, BLACK);
        
        // "Scan for more" below "Device info"
        const char* title = INTL_DISP_SCAN_FOR_MORE;
        int title_y = text_y + Font20.Height + 4;
        Paint_DrawString_Display(text_x, title_y, title, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        
        free(qr_bitmap_scaled);
    }

    // === CONTENT AREA ===
    uint16_t content_start_y = header_bottom_border_y + total_height + 20;

    // === INFORMATION PANEL (Single Column) ===
    uint16_t info_y = content_start_y;
    uint16_t line_spacing = 22; // Spacing between items
    uint16_t divider_spacing = 3; // Space before divider
    const int icon_size = 15; // Small icons for info items
    
    // Calculate label width for consistent value alignment (using Font16 for labels)
    const char* longest_label = "Robonomics Addr:";
    int label_width = strlen(longest_label) * Font16.Width;
    uint16_t icon_indent = content_left;
    uint16_t label_indent_after_icon = icon_indent + icon_size + 4;
    uint16_t value_indent = label_indent_after_icon + label_width - 2;
    int value_max_width = usable_width - value_indent - content_left;

    if (!cfg::standalone) {
        // Urban IP Address
        Paint_DrawImage(ip_address_15x15, icon_indent, info_y, icon_size, icon_size);
        Paint_DrawString_Display(label_indent_after_icon, info_y + (icon_size - Font16.Height) / 2, INTL_DISP_URBAN_IP, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
        String urban_display = urban_ip.length() > 0 ? urban_ip : INTL_DISP_NOT_CONNECTED;
        Paint_DrawString_Display(value_indent, info_y + (icon_size - Font12.Height) / 2, urban_display.c_str(), &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
        info_y += line_spacing;
        // Divider
        Paint_DrawLine(icon_indent, info_y + divider_spacing, usable_width - 8, info_y + divider_spacing, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        info_y += divider_spacing + 2;
    }

    // Altruist IP Address (Insight)
    Paint_DrawImage(ip_address_15x15, icon_indent, info_y, icon_size, icon_size);
    Paint_DrawString_Display(label_indent_after_icon, info_y + (icon_size - Font16.Height) / 2, INTL_DISP_INSIGHT_IP, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    String altruist_display = deviceStatus.ip_address.length() > 0 ? deviceStatus.ip_address : INTL_DISP_NOT_CONNECTED;
    Paint_DrawString_Display(value_indent, info_y + (icon_size - Font12.Height) / 2, altruist_display.c_str(), &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    info_y += line_spacing;
    // Divider
    Paint_DrawLine(icon_indent, info_y + divider_spacing, usable_width - 8, info_y + divider_spacing, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    info_y += divider_spacing + 2;

    // Firmware Version
    Paint_DrawImage(circuit_15x15, icon_indent, info_y, icon_size, icon_size);
    Paint_DrawString_Display(label_indent_after_icon, info_y + (icon_size - Font16.Height) / 2, INTL_FIRMWARE, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    String fw_version = String(SOFTWARE_VERSION_STR);
    Paint_DrawString_Display(value_indent, info_y + (icon_size - Font12.Height) / 2, fw_version.c_str(), &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    info_y += line_spacing;
    // Divider
    Paint_DrawLine(icon_indent, info_y + divider_spacing, usable_width - 8, info_y + divider_spacing, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    info_y += divider_spacing + 2;

    // SD Card Status - check dynamically to detect removal
#if defined(USE_SD_CARD)
    extern SDCard sdCardLogger;
    bool sd_card_currently_connected = sdCardLogger.checkInserted();
    // Update deviceStatus to reflect current state
    deviceStatus.sd_card_connected = sd_card_currently_connected;
#else
    bool sd_card_currently_connected = deviceStatus.sd_card_connected;
#endif
    Paint_DrawImage(sd_card_15x15, icon_indent, info_y, icon_size, icon_size);
    Paint_DrawString_Display(label_indent_after_icon, info_y + (icon_size - Font16.Height) / 2, INTL_DISP_SD_CARD, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    const char* sd_status = sd_card_currently_connected ? INTL_DISP_CONNECTED : INTL_DISP_NOT_CONNECTED;
    Paint_DrawString_Display(value_indent, info_y + (icon_size - Font12.Height) / 2, sd_status, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    info_y += line_spacing;
    // Divider
    Paint_DrawLine(icon_indent, info_y + divider_spacing, usable_width - 8, info_y + divider_spacing, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    info_y += divider_spacing + 2;

    // WiFi Status
    Paint_DrawImage(wifi_15x15, icon_indent, info_y, icon_size, icon_size);
    Paint_DrawString_Display(label_indent_after_icon, info_y + (icon_size - Font16.Height) / 2, INTL_DISP_WIFI_STATUS, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    bool wifi_connected = (WiFi.status() == WL_CONNECTED);
    const char* wifi_status = wifi_connected ? INTL_DISP_CONNECTED : INTL_DISP_DISCONNECTED;
    Paint_DrawString_Display(value_indent, info_y + (icon_size - Font12.Height) / 2, wifi_status, &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    info_y += line_spacing;
    // Divider
    Paint_DrawLine(icon_indent, info_y + divider_spacing, usable_width - 8, info_y + divider_spacing, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    info_y += divider_spacing + 2;

    // WiFi Name
    Paint_DrawImage(wifi_15x15, icon_indent, info_y, icon_size, icon_size);
    Paint_DrawString_Display(label_indent_after_icon, info_y + (icon_size - Font16.Height) / 2, INTL_DISP_WIFI_NAME, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    String wifi_name;
    if (wifi_connected) {
        String ssid = WiFi.SSID();
        if (ssid.length() == 0) {
            // Fallback to configured SSID if WiFi.SSID() is empty
            ssid = String(cfg::wlanssid);
        }
        wifi_name = ssid;
    } else {
        // Show configured SSID even when disconnected
        String ssid = String(cfg::wlanssid);
        if (ssid.length() > 0 && strcmp(cfg::wlanssid, WLANSSID) != 0) {
            wifi_name = ssid;
        } else {
            wifi_name = INTL_DISP_NOT_SET;
        }
    }
    Paint_DrawString_Display(value_indent, info_y + (icon_size - Font12.Height) / 2, wifi_name.c_str(), &Font12, &font_12_cyrillic, &font_12_ascii, WHITE, BLACK);
    info_y += line_spacing;
    // Divider
    Paint_DrawLine(icon_indent, info_y + divider_spacing, usable_width - 8, info_y + divider_spacing, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    info_y += divider_spacing + 2;

    // Robonomics Address (with word wrapping)
    Paint_DrawImage(location_15x15, icon_indent, info_y, icon_size, icon_size);
    Paint_DrawString_Display(label_indent_after_icon, info_y + (icon_size - Font16.Height) / 2, INTL_DISP_UNIQUE_ADDR, &Font16, &font_16_cyrillic, &font_16_ascii, WHITE, BLACK);
    String robonomics_display = robonomics_address.length() > 0 ? robonomics_address : INTL_DISP_NOT_SET;
    int robonomics_height = 0;
    drawWrappedText(value_indent, info_y + (icon_size - Font12.Height) / 2, robonomics_display, &Font12, WHITE, BLACK, value_max_width, robonomics_height);
}

#endif
