#ifdef ALTRUIST_INSIDE

#include "main_screen.h"
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include "../driver/DEV_Config.h"
#include "../driver/EPD.h"
// #include "graph.h"
#include <stdlib.h>
#include "utils.h"
#include "../icons/icons/icons_40x40.h"
#include "../icons/icons/icons_35x35.h"
#include "../../defines.h"
#include "../utils.h"
#include "display_common.h"


void drawValue(const char *label, float value, uint8_t precision, const unsigned char *image, const char *units, uint16_t image_size, uint16_t x_start, uint16_t y_start, uint16_t image_offset = 0) {
    Paint_DrawImage(image, x_start, y_start, image_size, image_size);
    Paint_DrawString_EN(x_start + image_size + image_offset, y_start, label, &Font12, WHITE, BLACK);
    char value_str[10];
    stringFromFloat(value_str, value, precision);
    Paint_DrawString_EN(x_start + image_size + image_offset, y_start + Font12.Height + 5, value_str, &Font20, WHITE, BLACK);
    Paint_DrawString_EN(x_start + image_size + Font20.Width * strlen(value_str) + image_offset, y_start + Font12.Height + 5 + Font20.Height / 4, units, &Font12, WHITE, BLACK);
}

void _parseJsonToStruct(const String &jsonString, main_screen_values_t &main_screen_values) {
    debug_outln_verbose(F("Got json string to display: "), jsonString);
    DynamicJsonDocument doc(2048);  // adjust size as needed

    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        debug_outln_info(F("deserializeJson() failed display: "), error.f_str());
        return;
    }

    JsonObject data = doc.as<JsonObject>();
    debug_outln_info(F("---"));
    // serializeJson(data, Serial);
    if (data.containsKey(ATRUIST_URBAN_SENSOR)) {
        if (data[ATRUIST_URBAN_SENSOR].containsKey("IP_address")) {
            main_screen_values.ip_address = data[ATRUIST_URBAN_SENSOR]["IP_address"]["value"].as<String>();
        }
        if (data[ATRUIST_URBAN_SENSOR].containsKey("SDS_P1")) {
            main_screen_values.pm10 = data[ATRUIST_URBAN_SENSOR]["SDS_P1"]["value"].as<float>();
        }
        if (data[ATRUIST_URBAN_SENSOR].containsKey("SDS_P2")) {
            main_screen_values.pm25 = data[ATRUIST_URBAN_SENSOR]["SDS_P2"]["value"].as<float>();
        }
        if (data[ATRUIST_URBAN_SENSOR].containsKey("BME280_humidity")) {
            main_screen_values.hum_outdoor = data[ATRUIST_URBAN_SENSOR]["BME280_humidity"]["value"].as<float>();
        }
        if (data[ATRUIST_URBAN_SENSOR].containsKey("BME280_temperature")) {
            main_screen_values.temp_outdoor = data[ATRUIST_URBAN_SENSOR]["BME280_temperature"]["value"].as<float>();
        }
        if (data[ATRUIST_URBAN_SENSOR].containsKey("BME280_pressure")) {
            main_screen_values.press_outdoor = data[ATRUIST_URBAN_SENSOR]["BME280_pressure"]["value"].as<float>() * 0.0075;
        }
        if (data[ATRUIST_URBAN_SENSOR].containsKey("PCBA_noiseMax")) {
            main_screen_values.noise_max = data[ATRUIST_URBAN_SENSOR]["PCBA_noiseMax"]["value"].as<float>();
        }
        if (data[ATRUIST_URBAN_SENSOR].containsKey("PCBA_noiseAvg")) {
            main_screen_values.noise_avg= data[ATRUIST_URBAN_SENSOR]["PCBA_noiseAvg"]["value"].as<float>();
        }
    }
    if (data.containsKey("SCD4x")) {
        main_screen_values.co2 = data["SCD4x"]["co2"]["value"].as<float>();
    }
    if (data.containsKey("BME680")) {
        main_screen_values.hum_indoor= data["BME680"]["humidity"]["value"].as<float>();
        main_screen_values.temp_indoor = data["BME680"]["temperature"]["value"].as<float>();
    }
}

void drawMainScreen(UBYTE *BlackImage, const String &jsonString, const String &device_ip_adrress) {
    main_screen_values_t main_screen_values;
    _parseJsonToStruct(jsonString, main_screen_values);

    drawValue("PM10", main_screen_values.pm10, 2, air_filter_35x35, "ppm", 35, 0, 35);
    drawValue("PM2.5", main_screen_values.pm25, 2, air_filter_35x35, "ppm", 35, 0, 80);

    drawValue("Noise Max", main_screen_values.noise_max, 0, volume_up_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24_35x35, "db", 35, 0, 125);
    drawValue("Noise Avg", main_screen_values.noise_avg, 0, volume_down_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24_35x35, "db", 35, 0, 170);

    drawValue("Temperature", main_screen_values.temp_outdoor, 1, wi_thermometer_cropped_35x35, "C", 35, 115, 57);
    drawValue("Humidity", main_screen_values.hum_outdoor, 1, wi_humidity_cropped_35x35, "%", 35, 115, 102);
    drawValue("Pressure", main_screen_values.press_outdoor, 0, wi_barometer_cropped_35x35, "mm/Hg", 35, 115, 147);

    Paint_DrawLine(235, 0, 235, 240, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    drawValue("Temperature", main_screen_values.temp_indoor, 1,  house_thermometer_40x40, "C", 40, 238, 57);
    drawValue("Humidity", main_screen_values.hum_indoor, 1,  house_humidity_40x40, "%", 40, 238, 102);
    drawValue("CO2", main_screen_values.co2, 1,  co2_svgrepo_com_35x35, "ppm", 35, 238, 147, 5);

    Paint_DrawRectangle(235 - main_screen_values.ip_address.length() * Font12.Width - 5, 3, 235, Font16.Height + Font12.Height + 8, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(235 - 7* Font16.Width - 5, 5, "Outdoor", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(235 - main_screen_values.ip_address.length() * Font12.Width - 5, Font16.Height + 5, main_screen_values.ip_address.c_str(), &Font12, BLACK, WHITE);


    Paint_DrawRectangle(360 - device_ip_adrress.length() * Font12.Width - 5, 3, 360, Font16.Height + Font12.Height + 8, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(360 - 6* Font16.Width - 5, 5, "Indoor", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(360 - device_ip_adrress.length() * Font12.Width - 5, Font16.Height + 5, device_ip_adrress.c_str(), &Font12, BLACK, WHITE);

    debug_outln_info(F("Draw main screen 6"));
    // refreshScreen(BlackImage);
}


#endif