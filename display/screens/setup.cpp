#ifdef ALTRUIST_INSIDE

#include <qrcode.h>
#include "../../config_manager/config_helpers.h"
#include "main_screen.h"
#include "../driver/EPD.h"
#include "setup.h"
#include "../../utils.h"
#include "display_common.h"

QRCode qrcode; 
unsigned char qr_bitmap[BITMAP_HEIGHT * (BITMAP_WIDTH / 8)];

void showSetupPage(UBYTE *BlackImage) {
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    char qr_data[100];
    sprintf(qr_data, "WIFI:S:%s;T:;P:;;", cfg::fs_ssid);
    qrcode_initText(&qrcode, qrcodeData, QR_VERSION, ECC_LOW, qr_data);
    
    // Debug: print QR data and size
    printf("QR Data: %s\n", qr_data);
    printf("QR Size: %d\n", qrcode.size);
    
    // Scale factor - try different values: 2, 3, 4, 5, 6
    int scale_factor = 4; // Start with 4x4 pixels per module
    
    // Calculate scaled dimensions
    int scaled_qr_width = qrcode.size * scale_factor;
    int scaled_qr_height = qrcode.size * scale_factor;
    
    // Add quiet zone (border) - fixed 3 pixels regardless of scale
    int quiet_zone = 3; // Always 3 pixels border
    int total_width = scaled_qr_width + (2 * quiet_zone);
    int total_height = scaled_qr_height + (2 * quiet_zone);
    
    // Create a bitmap that includes the quiet zone
    int qr_bitmap_width_bytes = (total_width + 7) / 8;
    int qr_bitmap_size = total_height * qr_bitmap_width_bytes;
    
    unsigned char *qr_bitmap_scaled = (unsigned char*)malloc(qr_bitmap_size);
    memset(qr_bitmap_scaled, 0x00, qr_bitmap_size); // White background (0x00)
    
    printf("Total bitmap size: %dx%d\n", total_width, total_height);

    // Draw scaled QR code with quiet zone offset
    for (uint8_t qr_y = 0; qr_y < qrcode.size; qr_y++) {
        for (uint8_t qr_x = 0; qr_x < qrcode.size; qr_x++) {
            if (qrcode_getModule(&qrcode, qr_x, qr_y)) {
                // Draw a scale_factor x scale_factor block for each QR module
                for (int sy = 0; sy < scale_factor; sy++) {
                    for (int sx = 0; sx < scale_factor; sx++) {
                        int pixel_x = quiet_zone + (qr_x * scale_factor) + sx;
                        int pixel_y = quiet_zone + (qr_y * scale_factor) + sy;
                        
                        // Bounds checking
                        if (pixel_x >= 0 && pixel_x < total_width && 
                            pixel_y >= 0 && pixel_y < total_height) {
                            qr_bitmap_scaled[pixel_y * qr_bitmap_width_bytes + (pixel_x / 8)] |= (0x80 >> (pixel_x % 8));
                        }
                    }
                }
            }
        }
    }

    int qr_x = DISPLAY_WIDTH / 2 - total_width / 2;
    int qr_y = DISPLAY_HEIGHT / 2 - total_height / 2 - 7;

    Paint_DrawString_EN(DISPLAY_WIDTH / 2 - 5*Font24.Width, qr_y / 2 - Font24.Height / 2, "WiFi Setup", &Font24, WHITE, BLACK);
    
    Paint_DrawImage(qr_bitmap_scaled, qr_x, qr_y, total_width, total_height);

    int label_y = qr_y + total_height + (DISPLAY_HEIGHT - (qr_y + total_height)) / 2 - (2 * Font16.Height + 5) / 2;

    Paint_DrawString_EN(DISPLAY_WIDTH / 2 - 5*Font16.Width, label_y, "Connect to", &Font16, WHITE, BLACK);
    Paint_DrawString_EN(DISPLAY_WIDTH / 2 - strlen(cfg::fs_ssid)*Font16.Width / 2, label_y + Font16.Height + 5, cfg::fs_ssid, &Font16, WHITE, BLACK);
    
    // Clean up
    free(qr_bitmap_scaled);
}

#endif