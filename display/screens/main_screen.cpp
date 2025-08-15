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
#include "../../config_manager/config_helpers.h"
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

    String urban_key = ATRUIST_URBAN_SENSOR;
    
    if (data.containsKey(urban_key)) {
        if (data[urban_key].containsKey("IP_address")) {
            main_screen_values.ip_address = data[urban_key]["IP_address"]["value"].as<String>();
        }
        if (data[urban_key].containsKey("SDS_P1")) {
            main_screen_values.pm10 = data[urban_key]["SDS_P1"]["value"].as<float>();
        }
        if (data[urban_key].containsKey("SDS_P2")) {
            main_screen_values.pm25 = data[urban_key]["SDS_P2"]["value"].as<float>();
        }
        if (data[urban_key].containsKey("BME280_humidity")) {
            main_screen_values.hum_outdoor = data[urban_key]["BME280_humidity"]["value"].as<float>();
        }
        if (data[urban_key].containsKey("BME280_temperature")) {
            main_screen_values.temp_outdoor = data[urban_key]["BME280_temperature"]["value"].as<float>();
        }
        if (data[urban_key].containsKey("BME280_pressure")) {
            main_screen_values.press_outdoor = data[urban_key]["BME280_pressure"]["value"].as<float>() * 0.0075;
        }
        if (data[urban_key].containsKey("PCBA_noiseMax")) {
            main_screen_values.noise_max = data[urban_key]["PCBA_noiseMax"]["value"].as<float>();
        }
        if (data[urban_key].containsKey("PCBA_noiseAvg")) {
            main_screen_values.noise_avg= data[urban_key]["PCBA_noiseAvg"]["value"].as<float>();
        }
    }
    if (data.containsKey("SCD4x")) {
        main_screen_values.co2 = data["SCD4x"]["co2"]["value"].as<float>();
    }
    if (data.containsKey("BME680")) {
        main_screen_values.hum_indoor= data["BME680"]["humidity"]["value"].as<float>();
        main_screen_values.temp_indoor = data["BME680"]["temperature"]["value"].as<float>();
        main_screen_values.press_indoor= data["BME680"]["pressure"]["value"].as<float>() * 0.0075;
    }
}

void drawMainScreen(UBYTE *BlackImage, const String &jsonString, const String &device_ip_adrress) {
    main_screen_values_t main_screen_values;
    _parseJsonToStruct(jsonString, main_screen_values);

    uint8_t values_part_border_height = 10;
    uint16_t up_line_height = Font16.Height + Font12.Height + 8;
    uint16_t values_part_height = DISPLAY_HEIGHT - up_line_height - 2*values_part_border_height;
    uint8_t value_item_height = Font12.Height + 5 + Font20.Height;
    uint8_t value_item_width = Font12.Width * 11 + 40;
    uint16_t column_width = DISPLAY_WIDTH / 3;
    uint8_t column_horizontal_interval;
    if (column_width > value_item_width) {
        column_horizontal_interval = (column_width - value_item_width) / 2;
    } else {
        column_horizontal_interval = 0;
    }

    uint8_t four_values_interval = (values_part_height - 4*value_item_height) / 3;
    uint8_t three_values_interval = (values_part_height - 3*value_item_height) / 2;

    // Рисуем одну темную полосу одинаковой ширины по всему верху экрана
    Paint_DrawRectangle(0, 0, DISPLAY_WIDTH, Font16.Height + Font12.Height + 8, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    
    // Рисуем текст на темной полосе
    Paint_DrawString_EN(5, 5, "Urban", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(5, Font16.Height + 5, main_screen_values.ip_address.c_str(), &Font12, BLACK, WHITE);

    Paint_DrawString_EN(2*column_width + 5, 5, "Insight", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(2*column_width + 5, Font16.Height + 5, device_ip_adrress.c_str(), &Font12, BLACK, WHITE);

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char date_str[18];
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", &timeinfo);

        int x = ((2*column_width - main_screen_values.ip_address.length() * Font12.Width - 10) - 16*Font12.Width)/2 + main_screen_values.ip_address.length() * Font12.Width + 10;
        Paint_DrawString_EN(x, 5, date_str, &Font12, BLACK, WHITE);
    }

    // Добавляем белые линии для разделения на 3 части внутри черной полосы
    Paint_DrawLine(2*column_width - 1, 0, 2*column_width -1, Font16.Height + Font12.Height + 8, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(main_screen_values.ip_address.length() * Font12.Width + 10 + 1, 0, main_screen_values.ip_address.length() * Font12.Width + 10 + 1, Font16.Height + Font12.Height + 8, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // 1 column

    drawValue("PM10", main_screen_values.pm10, 2, air_filter_35x35, "ppm", 35, 0, up_line_height + values_part_border_height);
    drawValue("PM2.5", main_screen_values.pm25, 2, air_filter_35x35, "ppm", 35, 0, up_line_height + values_part_border_height + four_values_interval + value_item_height);

    drawValue("Noise Max", main_screen_values.noise_max, 0, ear_hearing_35x35, "db", 35, 0, up_line_height + values_part_border_height + 2*four_values_interval + 2*value_item_height);
    drawValue("Noise Avg", main_screen_values.noise_avg, 0, ear_hearing_35x35, "db", 35, 0, up_line_height + values_part_border_height + 3*four_values_interval + 3*value_item_height);

    // 2 column
    
    drawValue("Temperature", main_screen_values.temp_outdoor, 1, wi_thermometer_cropped_35x35, "C", 35, column_width + column_horizontal_interval, up_line_height + values_part_border_height + values_part_height / 2 - four_values_interval - 1.5 * value_item_height);
    drawValue("Humidity", main_screen_values.hum_outdoor, 1, wi_humidity_cropped_35x35, "%", 35, column_width + column_horizontal_interval, up_line_height + values_part_border_height + values_part_height / 2 - value_item_height / 2);
    drawValue("Pressure", main_screen_values.press_outdoor, 0, pressure_40x40, "mm/Hg", 40, column_width + column_horizontal_interval, up_line_height + values_part_border_height + values_part_height / 2 + four_values_interval + value_item_height / 2);

    Paint_DrawLine(2*column_width, 0, 2*column_width, DISPLAY_HEIGHT, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // 3 column

    drawValue("Temperature", main_screen_values.temp_indoor, 1,  house_thermometer_40x40, "C", 40, 2*column_width + column_horizontal_interval, up_line_height + values_part_border_height);
    drawValue("Humidity", main_screen_values.hum_indoor, 1,  house_humidity_40x40, "%", 40, 2*column_width + column_horizontal_interval, up_line_height + values_part_border_height + four_values_interval + value_item_height);
    drawValue("Pressure", main_screen_values.press_indoor, 0,  pressure_40x40, "mm/Hg", 40, 2*column_width + column_horizontal_interval, up_line_height + values_part_border_height + 2*four_values_interval + 2*value_item_height);
    drawValue("CO2", main_screen_values.co2, 1,  co2_svgrepo_com_35x35, "ppm", 35, 2*column_width + column_horizontal_interval, up_line_height + values_part_border_height + 3*four_values_interval + 3*value_item_height, 5);
}


#endif