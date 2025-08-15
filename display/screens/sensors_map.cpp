#ifdef ALTRUIST_INSIDE

#include <qrcode.h>
#include "../../config_manager/config_helpers.h"
#include "main_screen.h"
#include "../driver/EPD.h"
#include "setup.h"
#include "../../utils.h"
#include "display_common.h"

QRCode QRSensorMap; 

void showSensorsMapPage(const String& robonomics_address) {
    uint8_t qrcodeData[qrcode_getBufferSize(12)];
    char qr_data[300];
    char lat[32], lon[32];
    const char* addr = robonomics_address.c_str();
    sscanf(cfg::coords_gps, "%31[^,],%31s", lat, lon);
    sprintf(qr_data, "https://sensors.social/#/remote/%s/17/%s/%s/%s", addr, lat, lon, addr);
    qrcode_initText(&QRSensorMap, qrcodeData, 12, ECC_LOW, qr_data);
    
    // Debug: print QR data and size
    printf("QR Data: %s\n", qr_data);
    printf("QR Size: %d\n", QRSensorMap.size);
    
    // Scale factor - try different values: 2, 3, 4, 5, 6
    int scale_factor = 3; // Start with 4x4 pixels per module
    
    // Calculate scaled dimensions
    int scaled_qr_width = QRSensorMap.size * scale_factor;
    int scaled_qr_height = QRSensorMap.size * scale_factor;
    
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
    for (uint8_t qr_y = 0; qr_y < QRSensorMap.size; qr_y++) {
        for (uint8_t qr_x = 0; qr_x < QRSensorMap.size; qr_x++) {
            if (qrcode_getModule(&QRSensorMap, qr_x, qr_y)) {
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

    Paint_DrawString_EN(DISPLAY_WIDTH / 2 - 5*Font24.Width, 0, "Sensors Map", &Font24, WHITE, BLACK);
    

    int qr_x = DISPLAY_WIDTH / 2 - total_width / 2;
    int qr_y = Font24.Height + (DISPLAY_HEIGHT - Font24.Height) / 2 - total_height / 2;
    
    Paint_DrawImage(qr_bitmap_scaled, qr_x, qr_y, total_width, total_height);
    // Clean up
    free(qr_bitmap_scaled);
}

#endif