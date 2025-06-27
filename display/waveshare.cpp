#include "waveshare.h"
#include <ArduinoJson.h>  // This was missing!
#include <string.h>       // For strlen
#include <stdio.h>
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "graph.h"
#include <stdlib.h>
#include "icons/icons/icons_40x40.h"
#include "icons/icons/icons_35x35.h"
#include "../defines.h"
#include "../utils.h"


void drawValue(const char *label, float value, uint8_t precision, const unsigned char *image, const char *units, uint16_t image_size, uint16_t x_start, uint16_t y_start, uint16_t image_offset = 0) {
    Paint_DrawImage(image, x_start, y_start, image_size, image_size);
    Paint_DrawString_EN(x_start + image_size + image_offset, y_start, label, &Font12, WHITE, BLACK);
    char value_str[10];
    stringFromFloat(value_str, value, precision);
    Paint_DrawString_EN(x_start + image_size + image_offset, y_start + Font12.Height + 5, value_str, &Font20, WHITE, BLACK);
    Paint_DrawString_EN(x_start + image_size + Font20.Width * strlen(value_str) + image_offset, y_start + Font12.Height + 5 + Font20.Height / 4, units, &Font12, WHITE, BLACK);
}

void createNewImage(UBYTE *&BlackImage) {
    UWORD Imagesize = ((EPD_3IN52_WIDTH % 8 == 0)? (EPD_3IN52_WIDTH / 8 ): (EPD_3IN52_WIDTH / 8 + 1)) * EPD_3IN52_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
    }
    Paint_NewImage(BlackImage, EPD_3IN52_WIDTH, EPD_3IN52_HEIGHT, 270, WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
}

void refreshScreen(UBYTE *&BlackImage) {
    DEV_Module_Init();
    EPD_3IN52_Init();
    EPD_3IN52_display_NUM(EPD_3IN52_WHITE);
    EPD_3IN52_lut_DU();
    EPD_3IN52_refresh();

    EPD_3IN52_SendCommand(0x50);
    EPD_3IN52_SendData(0x17);

    EPD_3IN52_display(BlackImage);
    EPD_3IN52_lut_GC();
    EPD_3IN52_refresh();
    DEV_Delay_ms(5000);

    printf("Clear...\r\n");
    // EPD_3IN52_Clear();
    
    // Sleep & close 5V
    printf("Goto Sleep...\r\n");
    EPD_3IN52_sleep();
}

void _parseJsonToStruct(const String &jsonString, main_screen_values_t &main_screen_values) {
    debug_outln_info(F("Got json string: "), jsonString);
    DynamicJsonDocument doc(1024);  // adjust size as needed

    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    JsonObject data = doc.as<JsonObject>();
    debug_outln_info(F("---"));
    serializeJson(data, Serial);
    if (data.containsKey(ATRUIST_URBAN_SENSOR)) {
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
    if (data.containsKey("SCD41")) {
        main_screen_values.co2 = data["SCD41"]["co2"]["value"].as<float>();
    }
    if (data.containsKey("BME680")) {
        main_screen_values.hum_indoor= data["BME680"]["humidity"]["value"].as<float>();
        main_screen_values.temp_indoor = data["BME680"]["temperature"]["value"].as<float>();
    }
}

void drawMainScreen(const String &jsonString) {
    UBYTE *BlackImage;
    createNewImage(BlackImage);
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

    Paint_DrawString_EN(235 - 7* Font16.Width - 5, 5, "Outdoor", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(360 - 6* Font16.Width - 5, 5, "Indoor", &Font16, BLACK, WHITE);

    refreshScreen(BlackImage);
}


// void setup() {
//     printf("EPD_3IN52_test Demo\r\n");
//     DEV_Module_Init();

//     printf("e-Paper Init and Clear...\r\n");
//     EPD_3IN52_Init();

//     EPD_3IN52_display_NUM(EPD_3IN52_WHITE);
//     EPD_3IN52_lut_DU();
//     EPD_3IN52_refresh();

//     EPD_3IN52_SendCommand(0x50);
//     EPD_3IN52_SendData(0x17);

//     DEV_Delay_ms(500);

//     //Create a new image cache
//     UBYTE *BlackImage;
//     /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
//     UWORD Imagesize = ((EPD_3IN52_WIDTH % 8 == 0)? (EPD_3IN52_WIDTH / 8 ): (EPD_3IN52_WIDTH / 8 + 1)) * EPD_3IN52_HEIGHT;
//     if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
//         printf("Failed to apply for black memory...\r\n");
//     }

//     printf("Paint_NewImage\r\n");
//     Paint_NewImage(BlackImage, EPD_3IN52_WIDTH, EPD_3IN52_HEIGHT, 270, WHITE);
//     // Paint_Clear(WHITE);
    
// #if 0   // GC waveform refresh 
//     Paint_SelectImage(BlackImage);
//     Paint_Clear(WHITE);
// 	Paint_DrawBitMap(gImage_3in52);
		
//     EPD_3IN52_display(BlackImage);
//     EPD_3IN52_lut_GC();
//     EPD_3IN52_refresh();
//     DEV_Delay_ms(5000);


// #endif

// #if 0  //DU waveform refresh
//     printf("Quick refresh is supported, but the refresh effect is not good, but it is not recommended\r\n");
//     Paint_SelectImage(BlackImage);
//     Paint_Clear(WHITE);
//     Paint_DrawBitMap(gImage_3in52);
		
//     EPD_3IN52_display(BlackImage);
//     EPD_3IN52_lut_DU();
//     EPD_3IN52_refresh();
//     DEV_Delay_ms(5000);

// #endif

//     printf("SelectImage:BlackImage\r\n");
//     Paint_SelectImage(BlackImage);
//     Paint_Clear(WHITE);

// #if 1 // Draw Values (var 2)
//     drawValue("PM10", 0.1, air_filter_35x35, "ppm", 35, 0, 35);
//     drawValue("PM2.5", 0.24, air_filter_35x35, "ppm", 35, 0, 80);

//     drawValue("Noise Max", 60, volume_up_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24_35x35, "db", 35, 0, 125);
//     drawValue("Noise Avg", 53, volume_down_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24_35x35, "db", 35, 0, 170);

//     drawValue("Temperature", 23.4, wi_thermometer_cropped_35x35, "C", 35, 115, 57);
//     drawValue("Humidity", 50.6, wi_humidity_cropped_35x35, "%", 35, 115, 102);
//     drawValue("Pressure", 745, wi_barometer_cropped_35x35, "mm/Hg", 35, 115, 147);

//     Paint_DrawLine(235, 0, 235, 240, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

//     drawValue("Temperature", 23.4, house_thermometer_40x40, "C", 40, 238, 57);
//     drawValue("Humidity", 50.6, house_humidity_40x40, "%", 40, 238, 102);
//     drawValue("CO2", 643, co2_svgrepo_com_35x35, "ppm", 35, 238, 147, 5);

//     Paint_DrawString_EN(235 - 7* Font16.Width - 5, 5, "Outdoor", &Font16, BLACK, WHITE);
//     Paint_DrawString_EN(360 - 6* Font16.Width - 5, 5, "Indoor", &Font16, BLACK, WHITE);

// #endif

// #if 0 // Draw Values (var 1)
//     drawValue("Temperature", 23.4, wi_thermometer_cropped_35x35, "C", 35, 0, 5);
//     drawValue("Humidity", 50.6, wi_humidity_cropped_35x35, "%", 35, 0, 50);
//     drawValue("Pressure", 745, wi_barometer_cropped_35x35, "mm/Hg", 35, 0, 95);
//     drawValue("PM10", 0.1, air_filter_35x35, "ppm", 35, 0, 140);
//     drawValue("PM2.5", 0.24, air_filter_35x35, "ppm", 35, 0, 185);

//     drawValue("Noise Max", 60, volume_up_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24_35x35, "db", 35, 120, 5);
//     drawValue("Noise Avg", 53, volume_down_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24_35x35, "db", 35, 220, 5);

//     Paint_DrawLine(122, 55, 122, 240, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
//     Paint_DrawLine(122, 55, 360, 55, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

//     drawValue("Temperature", 23.4, house_thermometer_40x40, "C", 40, 123, 65);
//     drawValue("Humidity", 50.6, house_humidity_40x40, "%", 40, 123, 105);
//     drawValue("CO2", 643, co2_svgrepo_com_35x35, "ppm", 35, 123, 150, 5);

// #endif

// #if 0 // Draw Graphs
//     printf("Drawing:BlackImage\r\n");
//     uint8_t values_count = 12;
//     uint32_t timestamps[values_count] = {1747729256, 1747732856, 1747736456, 1747740056, 1747743656, 1747747256, 1747750856, 1747754456, 1747758056, 1747761656, 1747765256, 1747768856};
//     float values1[values_count] = { 0.15, 0.2, 0.25, 0.5, 0.3, 0.25, 0.4, 0.43, 0.46, 0.2, 0.15, 0.5 };
//     float values11[values_count] = { 0.1, 0.05, 0.15, 0.25, 0.3, 0.45, 0.4, 0.43, 0.41, 0.28, 0.2, 0.15 };
//     float values2[values_count] = { 1500, 2000, 2500, 5000, 3000, 2500, 4000, 4300, 4600, 2000, 1500, 5000 };
//     float values3[values_count] = { 15, 20, 25, 50, 30, 25, 40, 43, 46, 20, 15, 50 };
//     float values4[values_count] = { 15, 2, 25, 5, 3, 25, 4, 43, 46, 2, 15, 5 };
//     float values5[values_count] = { 0.215, 0.22, 0.225, 0.25, 0.23, 0.225, 0.24, 0.243, 0.246, 0.22, 0.215, 0.25 };

//     uint16_t height = 100;
//     uint16_t width = 160;

//     Serial.println("Create graphs");
//     GraphPainter graph1(10, 110, height, width);
//     GraphPainter graph2(10, 230, height, width);
//     GraphPainter graph3(190, 110, height, width);
//     GraphPainter graph4(190, 230, height, width);

//     // graph1.setBlackMode();
//     // graph2.setBlackMode();
//     // graph3.setBlackMode();
//     // graph4.setBlackMode();

//     Serial.println("Create lines");
//     GraphLineStyle line_style1;
//     line_style1.width = DOT_PIXEL_2X2;
//     line_style1.use_main_color = true;
//     GraphLineStyle line_style11;
//     line_style11.width = DOT_PIXEL_1X1;
//     line_style11.use_main_color = false;
//     graph1.addLineValues(values1, timestamps, values_count, "PM10", line_style1);
//     graph1.addLineValues(values1, timestamps, values_count, "PM10", line_style11);
//     graph1.addLineValues(values11, timestamps, values_count, "PM2.5", line_style1);
//     graph2.addLineValues(values2, timestamps, values_count, "Hum", line_style1);
//     graph3.addLineValues(values3, timestamps, values_count, "Noise", line_style1);
//     graph4.addLineValues(values4, timestamps, values_count, "Temp", line_style1);

//     Serial.println("Draw graphs");
//     graph1.drawGraph();
//     graph2.drawGraph();
//     graph3.drawGraph();
//     graph4.drawGraph();
// #endif

//     printf("EPD_Display\r\n");
//     EPD_3IN52_display(BlackImage);
//     EPD_3IN52_lut_GC();
//     EPD_3IN52_refresh();
//     DEV_Delay_ms(5000);

//     printf("Clear...\r\n");
//     // EPD_3IN52_Clear();
    
//     // Sleep & close 5V
//     printf("Goto Sleep...\r\n");
//     EPD_3IN52_sleep();

//     free(BlackImage);
//     BlackImage = NULL;
//     DEV_Delay_ms(2000);//important, at least 2s
//     printf("close 5V, Module enters 0 power consumption ...\r\n");
// }

// /* The main loop -------------------------------------------------------------*/
// void loop()
// {
//   //
// }
